#define NOMINMAX
#define STB_IMAGE_IMPLEMENTATION
#include "Game.h"
#include "Shader.h"
#include "Texture.h"
#include "Audio.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>

// Helper Random Function
float randomFloat(float min, float max) {
    return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

Game::Game(unsigned int width, unsigned int height)
    : Width(width), Height(height), bgOffset(0.0f) // Αφαιρέσαμε τα Score, Level, State από εδώ
{
    // Αρχικοποίηση πλήκτρων
    for (int i = 0; i < 1024; ++i) Keys[i] = false;

    // Το gameData έχει δικό του constructor που τα βάζει όλα στο 0,
    // αλλά για σιγουριά τα δηλώνουμε κι εδώ:
    gameData.state = GAME_MENU;
    gameData.score = 0;
    gameData.highScore = 0;
    gameData.level = 1;
    gameData.lives = 3;
}

Game::~Game() {
    delete shader;
    delete texShip; delete texAlien; delete texSpace;
    delete texStart; delete texOver; delete texPause;
    delete texBullet;
    delete audio;
    delete texAlienHit;
}

void Game::Init() {
    // 1. Load Shaders & Textures
    shader = new Shader("Basic.vert", "Basic.frag");

    texShip = new Texture("spaceship.png");
    texAlien = new Texture("spacemine.png");
    texSpace = new Texture("space.jpg");
    texStart = new Texture("start.png");
    texOver = new Texture("gameover.png");
    texPause = new Texture("pause.png");
    texBullet = new Texture("bullet.png");
    texAlienHit = new Texture("minehit.png");

    // 2. Setup Audio
    audio = new Audio();
    audio->playMusic("music.mp3");

    // 3. Setup Quad Geometry
    float quadVertices[] = {
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f
    };
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);

    // 4. Init Gameplay Vars
    ResetLevel();

    gameData.state = GAME_MENU;
}

void Game::ResetLevel() {
    gameData.score = 0; gameData.level = 1; gameData.state = GAME_ACTIVE;
    gameData.lives = 3; // Επαναφορά ζωών
	immunityTimer = 0.0f;
    playerPos = glm::vec3(0.0f, -8.0f, 0.0f);
    enemies.clear(); bullets.clear();
    particles.clear();

    baseEnemySpeed = 2.0f;
    currentSpawnInterval = 1.5f;

}

void Game::SetKeys(int key, bool pressed) {
    if (key >= 0 && key < 1024) Keys[key] = pressed;
}

void Game::ProcessEvents(int key) {
    if (gameData.state == GAME_MENU && key == GLFW_KEY_ENTER) {
        ResetLevel();
    }
    if ((gameData.state == GAME_OVER || gameData.state == GAME_PAUSED) && key == GLFW_KEY_R) {
        ResetLevel();
    }
    if (key == GLFW_KEY_P) {
        if (gameData.state == GAME_ACTIVE) gameData.state = GAME_PAUSED;
        else if (gameData.state == GAME_PAUSED) gameData.state = GAME_ACTIVE;
    }
}

void Game::ProcessInput(float dt) {
    if (gameData.state == GAME_ACTIVE) {
        float velocity = 12.0f * dt;
        if (Keys[GLFW_KEY_L] && playerPos.x < 8.0f) playerPos.x += velocity;
        if (Keys[GLFW_KEY_J] && playerPos.x > -8.0f) playerPos.x -= velocity;

        // Shooting
        static float shootCooldown = 0.2f;
        float currentTime = glfwGetTime();
        if (Keys[GLFW_KEY_SPACE] && currentTime - lastShootTime >= shootCooldown) {
            Bullet b; b.position = playerPos + glm::vec3(0, 1.5f, 0); b.active = true;
            bullets.push_back(b);
            lastShootTime = currentTime;
            audio->play("shoot.mp3");
        }
    }
}

void Game::Update(float dt) {
    // Background Scroll
    if (gameData.state != GAME_PAUSED) {
        // Κινούμαστε προς τα ΚΑΤΩ (αρνητικά)
        bgOffset -= 2.0f * dt; 
        
        // Αν κατεβήκαμε 20 μονάδες (ένα πλήρες ύψος εικόνας), κάνουμε reset
        if (bgOffset <= -15.0f) {
            bgOffset += 10.0f;
        }
    }

    if (gameData.state == GAME_ACTIVE) {
        float currentTime = glfwGetTime();

        // Level Up Logic
        int newLevel = 1 + (gameData.score / 500);
        if (newLevel > gameData.level) {
            gameData.level = newLevel;

            // 1. ΤΑΧΥΤΗΤΑ ΚΑΘΟΔΟΥ:
            // Αυξάνεται πιο αργά (0.2 αντί για 0.4) και σταματάει στο 6.0 (Cap).
            baseEnemySpeed = std::min(6.0f, 2.0f + (gameData.level * 0.3f));

            // 2. ΣΥΧΝΟΤΗΤΑ SPAWN:
            // Δεν αφήνουμε να πέσει κάτω από 0.5 δευτερόλεπτα (για να μην γεμίσει η οθόνη)
            currentSpawnInterval = std::max(0.5f, 1.5f - (gameData.level * 0.2f));

        }

        // Spawner
        if (currentTime - lastSpawnTime >= currentSpawnInterval) {
            Enemy e;
            float randomX = randomFloat(-7.5f, 7.5f);
            e.position = glm::vec3(randomX, 12.0f, 0.0f);
            e.originalX = randomX;
            e.speed = baseEnemySpeed;
            e.wobbleOffset = randomFloat(0.0f, 3.14f);

  
            // Level 1-3: 1 HP
            // Level 4-6: 2 HP
            // Level 7+: 3 HP
            e.hp = 1 + (gameData.level / 2);
            e.flashTimer = 0.1f;
            

            enemies.push_back(e);
            lastSpawnTime = currentTime;
        }

        // Updates & Collisions
        for (auto& b : bullets) if (b.active) b.position.y += 18.0f * dt;

        for (auto& e : enemies) {
            e.position.y -= e.speed * dt;
            if (e.flashTimer > 0.0f) e.flashTimer -= dt;

            if (gameData.level == 1) {
                e.position.x = e.originalX;
            }
            else {
                // ZIG-ZAG LOGIC ME ORIO

                // Πλάτος (Intensity): Σταματάει στο 3.0 (δεν πάει άκρη-άκρη)
                float intensity = std::min(3.0f, 1.0f + (gameData.level * 0.2f));

                // Συχνότητα (Frequency): Σταματάει στο 4.0 (για να μην τρέμει σαν τρελό)
                float frequency = std::min(4.0f, 2.0f + (gameData.level * 0.15f));

                float offsetX = sin(currentTime * frequency + e.wobbleOffset) * intensity;
                float newX = e.originalX + offsetX;

                // Απόλυτο όριο οθόνης
                if (newX > 8.0f) newX = 8.0f;
                if (newX < -8.0f) newX = -8.0f;

                e.position.x = newX;
            }
        }

        // Collision Detection
        bool scoreChanged = false;
        for (auto& b : bullets) {
            if (!b.active) continue;
            for (auto& e : enemies) { // Χρησιμοποιούμε αναφορά &e για να αλλάξουμε το HP
                if (glm::distance(b.position, e.position) < 1.2f) {
                    b.active = false; // Η σφαίρα χάνεται πάντα

                    // ΜΕΙΩΣΗ ΖΩΗΣ
                    e.hp--;
                    e.flashTimer = 0.1f; // Άναψε λευκό για 0.1 δευτερόλεπτα

                    // ΑΝ ΠΕΘΑΝΕ
                    if (e.hp <= 0) {
                        // Τον μετακινούμε μακριά για να σβηστεί από το cleanup (τρικ για να μην χαλάσει το loop)
                        SpawnExplosion(e.position);
                        e.position.y = -50.0f;
                       

                        gameData.score += 50; // <--- Χρησιμοποιούμε το gameData
                        if (gameData.score > gameData.highScore) gameData.highScore = gameData.score;
                        scoreChanged = true;
                        std::cout << "Enemy Killed! Score is now: " << gameData.score << std::endl;

                        audio->play("explosion.mp3");
                    }
                    else {
                         audio->play("hit.mp3"); 
                    }

                    break; // Μια σφαίρα χτυπάει έναν εχθρό
                }
            }
        }

        // --- UPDATE PARTICLES ---
        for (auto& p : particles) {
            p.position += p.velocity * dt; // Κίνηση
            p.life -= dt * 2.0f;           // Μείωση ζωής (σβήνει γρήγορα)
        }
        // Διαγραφή νεκρών σωματιδίων
        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](Particle& p) { return p.life <= 0.0f; }), particles.end());

        // Μείωση χρόνου αθανασίας
        if (immunityTimer > 0.0f) {
            immunityTimer -= dt;
        }

        // Game Over / Damage Check (Player vs Enemy)
        for (int i = 0; i < enemies.size(); i++) {
            // Έλεγχος σύγκρουσης
            if (abs(enemies[i].position.y - playerPos.y) < 1.5f &&
                abs(enemies[i].position.x - playerPos.x) < 1.5f) {

                // Χτυπάμε ΜΟΝΟ αν δεν είμαστε αθάνατοι
                if (immunityTimer <= 0.0f) {
                    gameData.lives--; // Μείωσε ζωή
                    audio->play("explosion.mp3"); // Ήχος πόνου/έκρηξης

                    // Δημιούργησε έκρηξη στον παίκτη για εφέ
                    SpawnExplosion(playerPos);

                    if (gameData.lives > 0) {
                        // Αν έχουμε κι άλλες ζωές: Γίνομαστε αθάνατοι για 2 δευτερόλεπτα
                        immunityTimer = 2.0f;

                        // Καθαρίζουμε τους κοντινούς εχθρούς για να μην μας ξαναχτυπήσουν αμέσως (Fair Play)
                        enemies.erase(enemies.begin() + i);
                        i--; // Διόρθωση δείκτη μετά τη διαγραφή
                    }
                    else {
                        // Αν τελείωσαν οι ζωές: ΤΕΛΟΣ
                        gameData.state = GAME_OVER;
                        audio->play("gameover.mp3");
                    }
                }
            }
        }


        // Cleanup
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](Bullet& b) { return b.position.y > 20.0f || !b.active; }), bullets.end());
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](Enemy& e) { return e.position.y < -12.0f; }), enemies.end());


    }
}

void Game::Render() {
    shader->use();

    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)Width / Height, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 18), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    shader->setMat4("projection", proj);
    shader->setMat4("view", view);

    glBindVertexArray(VAO);

    // 1. Draw Background
    texSpace->bind();
    shader->setBool("useTexture", true);


    // Κομμάτι 1: Ξεκινάει στο κέντρο (0) και κατεβαίνει (bgOffset)
    // Καλύπτει από Υ=-10 έως Υ=10
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, bgOffset, -5.0f));
    model = glm::scale(model, glm::vec3(30.0f, 20.0f, 1.0f));
    shader->setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Κομμάτι 2: Ξεκινάει από πάνω (20) και κατεβαίνει μαζί με το πρώτο
    // Καλύπτει από Υ=10 έως Υ=30 (έτοιμο να μπει στην οθόνη)
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, bgOffset + 20.0f, -5.0f));
    model = glm::scale(model, glm::vec3(30.0f, 20.0f, 1.0f));
    shader->setMat4("model", model);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2. Draw Game Objects
    if (gameData.state != GAME_MENU) {
        if (gameData.state != GAME_OVER) {
            if (immunityTimer <= 0.0f || (int)(immunityTimer * 15.0f) % 2 == 0) {
                texShip->bind();
                shader->setBool("useTexture", true);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), playerPos);
                model = glm::scale(model, glm::vec3(3.0f, 3.0f, 1.0f));
                shader->setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            texAlien->bind();
            for (auto& e : enemies) {
                // ΛΟΓΙΚΗ DAMAGE SPRITE:
                // Αν ο εχθρός έχει χτυπηθεί πρόσφατα (flashTimer > 0),
                // βάλε την εικόνα "hit". Αλλιώς βάλε την κανονική.
                if (e.flashTimer > 0.0f) {
                    texAlienHit->bind();
                }
                else {
                    texAlien->bind();
                }

                // Σιγουρέψου ότι το flash του shader είναι κλειστό (αν το είχες κρατήσει)
                shader->setBool("flash", false);

                glm::mat4 model = glm::translate(glm::mat4(1.0f), e.position);
                model = glm::scale(model, glm::vec3(2.0f, 2.0f, 1.0f));
                shader->setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            // ΕΠΑΝΑΦΟΡΑ: Κλείσε το flash για τα επόμενα αντικείμενα!
            shader->setBool("flash", false);

            // DRAW PARTICLES
            shader->setBool("useTexture", false); // Θέλουμε χρώμα, όχι εικόνα
            for (auto& p : particles) {
                shader->setVec3("myColor", p.color.x, p.color.y, p.color.z);

                glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
                // Τρικ: Όσο μειώνεται η ζωή (life), μικραίνει το μέγεθος (scale)
                // Έτσι φαίνεται σαν να σβήνει!
                float size = p.life * 0.4f;
                model = glm::scale(model, glm::vec3(size, size, 1.0f));

                shader->setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            texBullet->bind(); // 1. Bind την εικονα της σφαιρας
            shader->setBool("useTexture", true); // 2. Ενεργοποιηση Texture (Πριν ηταν false)

            for (auto& b : bullets) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), b.position);

                // ΑΛΛΑΓΗ SCALE:
                // Πριν ηταν (0.3f, 0.8f, 1.0f) -> Μακροστενο
                // Τωρα το κανουμε (0.6f, 0.6f, 1.0f) -> Τετραγωνο (για να φαινεται στρογγυλη η μπαλα)
                model = glm::scale(model, glm::vec3(0.6f, 0.6f, 1.0f));

                shader->setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }

    // 3. UI Overlays
    glEnable(GL_BLEND);
    if (gameData.state == GAME_MENU) {
        texStart->bind(); shader->setBool("useTexture", true);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
        model = glm::scale(model, glm::vec3(12.0f, 6.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else if (gameData.state == GAME_PAUSED) {
        texPause->bind(); shader->setBool("useTexture", true);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
        model = glm::scale(model, glm::vec3(8.0f, 4.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else if (gameData.state == GAME_OVER) {
        texOver->bind(); shader->setBool("useTexture", true);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
        model = glm::scale(model, glm::vec3(10.0f, 5.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // --- UI: LIVES ---
            // Ζωγραφίζουμε μικρά αεροπλανάκια πάνω δεξιά
    texShip->bind(); // Χρησιμοποιούμε το ίδιο texture
    shader->setBool("useTexture", true);
    for (int i = 0; i < gameData.lives; i++) {
        // Τοποθέτηση: Πάνω δεξιά (X: 6.0, 7.0, 8.0..., Y: 8.0)
        // Υπολογίζουμε τη θέση ώστε να είναι στη σειρά
        float xPos = 5.5f + (i * 1.2f);
        float yPos = 8.0f; // Ψηλά στην οθόνη

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(xPos, yPos, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); // Μικρούλια
        shader->setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void Game::SpawnExplosion(glm::vec3 position) {
    // Δημιουργούμε 20 σωματίδια για κάθε έκρηξη
    for (int i = 0; i < 20; i++) {
        Particle p;
        p.position = position;

        // Τυχαία ταχύτητα προς όλες τις κατευθύνσεις (Explosion effect)
        float velX = randomFloat(-5.0f, 5.0f);
        float velY = randomFloat(-5.0f, 5.0f);
        p.velocity = glm::vec3(velX, velY, 0.0f);

        // Χρώμα: Πορτοκαλί/Κίτρινο/Κόκκινο τυχαία
        float r = 1.0f;
        float g = randomFloat(0.5f, 1.0f); // Κίτρινο προς Πορτοκαλί
        float b = 0.0f;
        p.color = glm::vec3(r, g, b);

        p.life = 1.0f; // Ζει για 1 δευτερόλεπτο

        particles.push_back(p);
    }
}