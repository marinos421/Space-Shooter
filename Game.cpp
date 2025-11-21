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
    : Width(width), Height(height), State(GAME_MENU), Score(0), HighScore(0), Level(1), bgOffset(0.0f)
{
    // Arxikopoiisi pliktrwn
    for (int i = 0; i < 1024; ++i) Keys[i] = false;
}

Game::~Game() {
    delete shader;
    delete texShip; delete texAlien; delete texSpace;
    delete texStart; delete texOver; delete texPause;
    delete audio;
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

    State = GAME_MENU;
}

void Game::ResetLevel() {
    Score = 0; Level = 1; State = GAME_ACTIVE;
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
    if (State == GAME_MENU && key == GLFW_KEY_ENTER) {
        ResetLevel();
    }
    if ((State == GAME_OVER || State == GAME_PAUSED) && key == GLFW_KEY_R) {
        ResetLevel();
    }
    if (key == GLFW_KEY_P) {
        if (State == GAME_ACTIVE) State = GAME_PAUSED;
        else if (State == GAME_PAUSED) State = GAME_ACTIVE;
    }
}

void Game::ProcessInput(float dt) {
    if (State == GAME_ACTIVE) {
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
    if (State != GAME_PAUSED) {
        // Κινούμαστε προς τα ΚΑΤΩ (αρνητικά)
        bgOffset -= 2.0f * dt; 
        
        // Αν κατεβήκαμε 20 μονάδες (ένα πλήρες ύψος εικόνας), κάνουμε reset
        if (bgOffset <= -15.0f) {
            bgOffset += 10.0f;
        }
    }

    if (State == GAME_ACTIVE) {
        float currentTime = glfwGetTime();

        // Level Up Logic
        int newLevel = 1 + (Score / 500);
        if (newLevel > Level) {
            Level = newLevel;
            baseEnemySpeed = 2.0f + (Level * 0.4f);
            currentSpawnInterval = std::max(0.4f, 1.5f - (Level * 0.15f));
        }

        // Spawner
        if (currentTime - lastSpawnTime >= currentSpawnInterval) {
            Enemy e;
            float randomX = randomFloat(-7.5f, 7.5f);
            e.position = glm::vec3(randomX, 12.0f, 0.0f);
            e.originalX = randomX;
            e.speed = baseEnemySpeed;
            e.wobbleOffset = randomFloat(0.0f, 3.14f);
            enemies.push_back(e);
            lastSpawnTime = currentTime;
        }

        // Updates & Collisions
        for (auto& b : bullets) if (b.active) b.position.y += 18.0f * dt;

        for (auto& e : enemies) {
            e.position.y -= e.speed * dt;
            if (Level > 1) {
                float intensity = std::min(3.5f, 1.0f + (Level * 0.3f));
                float frequency = 2.5f + (Level * 0.1f);
                float newX = e.originalX + sin(currentTime * frequency + e.wobbleOffset) * intensity;
                if (newX > 8.5f) newX = 8.5f; if (newX < -8.5f) newX = -8.5f;
                e.position.x = newX;
            }
            else {
                e.position.x = e.originalX;
            }
        }

        // Collision Detection
        for (auto& b : bullets) {
            if (!b.active) continue;
            for (int i = 0; i < enemies.size(); i++) {
                if (glm::distance(b.position, enemies[i].position) < 1.2f) {
                    b.active = false;
                    SpawnExplosion(enemies[i].position);
                    enemies.erase(enemies.begin() + i);
                    Score += 10;
                    if (Score > HighScore) HighScore = Score;
                    audio->play("explosion.mp3");
                    i--; break;
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

        // Game Over Check
        for (auto& e : enemies) {
            if (abs(e.position.y - playerPos.y) < 1.5f && abs(e.position.x - playerPos.x) < 1.5f) {
                State = GAME_OVER;
                audio->play("gameover.mp3");
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
    if (State != GAME_MENU) {
        if (State != GAME_OVER) {
            texShip->bind();
            model = glm::translate(glm::mat4(1.0f), playerPos);
            model = glm::scale(model, glm::vec3(3.0f, 3.0f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);

            texAlien->bind();
            for (auto& e : enemies) {
                model = glm::translate(glm::mat4(1.0f), e.position);
                model = glm::scale(model, glm::vec3(2.0f, 2.0f, 1.0f));
                shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
            }

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

            shader->setBool("useTexture", false); shader->setVec3("myColor", 1.0f, 0.8f, 0.0f);
            for (auto& b : bullets) {
                model = glm::translate(glm::mat4(1.0f), b.position);
                model = glm::scale(model, glm::vec3(0.3f, 0.8f, 1.0f));
                shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }

    // 3. UI Overlays
    glEnable(GL_BLEND);
    if (State == GAME_MENU) {
        texStart->bind(); shader->setBool("useTexture", true);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
        model = glm::scale(model, glm::vec3(12.0f, 6.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else if (State == GAME_PAUSED) {
        texPause->bind(); shader->setBool("useTexture", true);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
        model = glm::scale(model, glm::vec3(8.0f, 4.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else if (State == GAME_OVER) {
        texOver->bind(); shader->setBool("useTexture", true);
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f));
        model = glm::scale(model, glm::vec3(10.0f, 5.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
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