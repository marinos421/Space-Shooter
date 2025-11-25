#define NOMINMAX
#define STB_IMAGE_IMPLEMENTATION
#include "Game.h"
#include "Shader.h"
#include "Texture.h"
#include "Audio.h"
// Μην ξεχασεις να κανεις include τον manager
#include "EnemyManager.h" 
#include "Commons.h"
#include "fstream"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>

float randomFloat(float min, float max) {
    return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

Game::Game(unsigned int width, unsigned int height)
    : Width(width), Height(height), bgOffset(0.0f)
{
    for (int i = 0; i < 1024; ++i) Keys[i] = false;
}

Game::~Game() {
    delete shader;
    // Delete textures (Alien textures are deleted inside EnemyManager)
    delete texShip; delete texSpace;
    delete texStart; delete texOver; delete texPause;
    delete texBullet;
    delete texPowTriple; delete texPowShield;
    delete texPowPiercing;
    delete audio;

    // DELETE MANAGER
    delete enemyManager;
}

void Game::Init() {
    shader = new Shader("Basic.vert", "Basic.frag");

    texShip = new Texture("spaceship.png");
    texSpace = new Texture("space.jpg");
    texStart = new Texture("start.png");
    texOver = new Texture("gameover.png");
    texPause = new Texture("pause.png");
    texBullet = new Texture("bullet.png");

    texPowTriple = new Texture("powerup_triple.png");
    texPowShield = new Texture("powerup_shield.png");
	texPowPiercing = new Texture("powerup_piercing.png");

    // INIT ENEMY MANAGER
    enemyManager = new EnemyManager();
    enemyManager->Init();

    audio = new Audio();
    audio->playMusic("music.mp3");

    float quadVertices[] = {
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f
    };
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);

    ResetLevel();
    LoadHighScore();
    gameData.state = GAME_MENU;
    //gameData.level = 9;
}

void Game::ResetLevel() {
    gameData.score = 0; gameData.level = 1; gameData.state = GAME_ACTIVE;
    gameData.lives = 3;
    gameData.tripleShotTimer = 0.0f;
    immunityTimer = 0.0f;
    gameData.comboCount = 0;
    gameData.comboTimer = 0.0f;
    gameData.multiplier = 1.0f;

    playerPos = glm::vec3(0.0f, -8.0f, 0.0f);
    bullets.clear(); particles.clear(); powerups.clear();
    enemyBullets.clear();

    // RESET MANAGER
    enemyManager->Clear();
}

void Game::SetKeys(int key, bool pressed) { if (key >= 0 && key < 1024) Keys[key] = pressed; }

void Game::ProcessEvents(int key) {
    if (gameData.state == GAME_MENU && key == GLFW_KEY_ENTER) ResetLevel();
    if ((gameData.state == GAME_OVER || gameData.state == GAME_PAUSED) && key == GLFW_KEY_R) ResetLevel();
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

        static float shootCooldown = 0.2f;
        float currentTime = glfwGetTime();
        if (Keys[GLFW_KEY_SPACE] && currentTime - lastShootTime >= shootCooldown) {

            // Υπολογισμός Penetration
            int pen = 1;
            if (gameData.piercingTimer > 0.0f) pen = 100; // Piercing mode!

            if (gameData.tripleShotTimer > 0.0f) {
                // ΠΡΟΣΘΕΣΕ ΤΟ pen ΣΕ ΟΛΕΣ!
                Bullet b1; b1.position = playerPos + glm::vec3(0, 1.5f, 0); b1.active = true;
                b1.velocity = glm::vec3(0.0f, 18.0f, 0.0f);
                b1.penetration = pen; // <--- FIX
                bullets.push_back(b1);

                Bullet b2; b2.position = playerPos + glm::vec3(0.5f, 1.5f, 0); b2.active = true;
                b2.velocity = glm::vec3(5.0f, 18.0f, 0.0f);
                b2.penetration = pen; // <--- FIX
                bullets.push_back(b2);

                Bullet b3; b3.position = playerPos + glm::vec3(-0.5f, 1.5f, 0); b3.active = true;
                b3.velocity = glm::vec3(-5.0f, 18.0f, 0.0f);
                b3.penetration = pen; // <--- FIX
                bullets.push_back(b3);
            }
            else {
                Bullet b; b.position = playerPos + glm::vec3(0, 1.5f, 0); b.active = true;
                b.velocity = glm::vec3(0.0f, 18.0f, 0.0f);
                b.penetration = pen; // <--- FIX
                bullets.push_back(b);
            }
            lastShootTime = currentTime;
            audio->play("shoot.mp3");
        }
    }
}

void Game::SpawnPowerUp(glm::vec3 position) {
    int chance = rand() % 100;
    if (chance < 10) {
        PowerUpType type;
        int typeRnd = rand() % 4;
        if (typeRnd == 0) type = POWER_LIFE;
        else if (typeRnd == 1) type = POWER_TRIPLE;
		else if (typeRnd == 2) type = POWER_PIERCING;
        else type = POWER_SHIELD;
        powerups.push_back(PowerUp(position, type));
    }
}

void Game::Update(float dt) {
    if (gameData.state != GAME_PAUSED) {
        bgOffset -= 2.0f * dt;
        if (bgOffset <= -15.0f) bgOffset += 10.0f;
    }

    if (gameData.state == GAME_ACTIVE) {
        float currentTime = glfwGetTime();

        if (gameData.comboTimer > 0.0f) {
            gameData.comboTimer -= dt;
            if (gameData.comboTimer <= 0.0f) {
                // Ο χρόνος τελείωσε! Χάνεις το combo.
                gameData.comboCount = 0;
                gameData.multiplier = 1.0f;
                std::cout << "Combo Lost!" << std::endl; // Debug
            }
        }

        if (immunityTimer > 0.0f) immunityTimer -= dt;
        if (gameData.tripleShotTimer > 0.0f) gameData.tripleShotTimer -= dt;
        if (gameData.piercingTimer > 0.0f) gameData.piercingTimer -= dt;

        // Level Logic
        int newLevel = 1 + (gameData.score / 500);
        if (newLevel > gameData.level) {
            gameData.level = newLevel;
        }

        // --- ENEMY MANAGER UPDATE ---
        // Εδω γινεται ολη η δουλεια (Spawn, Movement, Cleanup)
        // Περναμε τα bullets εστω και αν δεν χρησιμοποιουνται ακομα για εχθρικα πυρα
        enemyManager->Update(dt, currentTime, gameData.level, playerPos, enemyBullets);

        // Updates
        for (auto& b : bullets) if (b.active) b.position += b.velocity * dt;
        for (auto& b : enemyBullets) if (b.active) b.position += b.velocity * dt;
        for (auto& p : powerups) if (p.active) p.position.y -= p.velocityY * dt;
        for (auto& p : particles) { p.position += p.velocity * dt; p.life -= dt * 2.0f; }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](Particle& p) { return p.life <= 0.0f; }), particles.end());

        // --- COLLISION: BULLET vs ENEMY ---
                // Παιρνουμε αναφορά στη λιστα εχθρων απο τον Manager
        std::vector<Enemy>& enemies = enemyManager->GetEnemies();
        std::vector<Enemy> spawnedMinions;

        for (auto& b : bullets) {
            if (!b.active) continue;
            for (auto& e : enemies) {

                // --- ΔΙΟΡΘΩΣΗ HITBOX ---
                float hitRadius = 1.5f; // Βασική ακτίνα για μικρούς

                // Αν είναι Boss, μεγαλώνουμε το στόχο!
                if (e.type == ENEMY_BOSS_MINE || e.type == ENEMY_BOSS_SHIP) {
                    hitRadius = 4.5f; // Μεγάλη ακτίνα για το Boss (ανάλογη του Scale 5.0)
                }
                // -----------------------

                // Χρησιμοποιούμε το hitRadius αντί για το σκέτο 1.2f
                if (glm::distance(b.position, e.position) < hitRadius) {

                    // PIERCING LOGIC
                    b.penetration--;
                    if (b.penetration <= 0) {
                        b.active = false;
                    }
                
                    e.hp--; // <--- ΑΥΤΟ ΜΕΙΩΝΕΙ ΤΗ ΖΩΗ ΤΟΥ BOSS ΚΑΝΟΝΙΚΑ
                    e.flashTimer = 0.1f;

                    if (e.hp <= 0) {
                        SpawnExplosion(e.position);
                        SpawnPowerUp(e.position);
                        glm::vec3 deathPos = e.position;
                        e.position.y = -50.0f; // Τον διώχνουμε για να διαγραφεί

                        // --- 1. ΥΠΟΛΟΓΙΣΜΟΣ ΠΟΝΤΩΝ (BASE SCORE) ---
                        int basePoints = 10; // Οι απλές νάρκες (Basic)

                        if (e.type == ENEMY_SHOOTER) {
                            basePoints = 20; // Τα πλοία που πυροβολούν αξίζουν διπλά

                        }
                        else if (e.type == ENEMY_SPLITTER && e.splitLevel == 0) {
                            for (int k = 0; k < 2; k++) {
                                Enemy mini;
                                mini.position = deathPos; // Χρήση της σωστής θέσης!
                                mini.originalX = deathPos.x;

                                float dirX = (k == 0) ? -1.5f : 1.5f;
                                mini.wobbleOffset = dirX;

                                // Initialize Speed! (Fixing garbage value)
                                mini.speed = 1.5f;

                                mini.hp = 1;
                                mini.maxHp = 1;
                                mini.splitLevel = 1;
                                mini.type = ENEMY_SPLITTER;
                                mini.flashTimer = 0.0f;
                                mini.weaponTimer = 0.0f;

                                spawnedMinions.push_back(mini);
                            }
                        }
                        else if (e.type == ENEMY_BOSS_MINE || e.type == ENEMY_BOSS_SHIP) {
                            basePoints = 1000; // Τα Boss δίνουν πολλά για να αλλάξεις Level
                        }

                        // --- 2. ΕΦΑΡΜΟΓΗ MULTIPLIER ---
                        // Τύπος: Πόντοι = Βάση * Πολλαπλασιαστής
                        int finalPoints = (int)(basePoints * gameData.multiplier);
                        gameData.score += finalPoints;

                        // --- 3. ΕΝΗΜΕΡΩΣΗ COMBO (Μόνο για μικρούς εχθρούς) ---
                        if (e.type != ENEMY_BOSS_MINE && e.type != ENEMY_BOSS_SHIP) {
                            gameData.comboCount++;       // Αυξάνουμε το σερί kills
                            gameData.comboTimer = 3.0f;  // Έχεις 3 δευτερόλεπτα για τον επόμενο!

                            // Κάθε kill ανεβάζει τον πολλαπλασιαστή κατά 0.1
                            gameData.multiplier = 1.0f + (gameData.comboCount * 0.1f);

                            // Βάζουμε ταβάνι (Cap) στο x5.0 για να μην ξεφύγει
                            if (gameData.multiplier > 5.0f) gameData.multiplier = 5.0f;
                        }

                        // --- 4. POWER UPS (Όπως τα είχες) ---
                        if (e.type == ENEMY_BOSS_MINE || e.type == ENEMY_BOSS_SHIP) {
                            SpawnPowerUp(e.position);
                            SpawnPowerUp(e.position + glm::vec3(2.0f, 0.0f, 0.0f));
                        }
                        else {
                            SpawnPowerUp(e.position);
                        }

                        // --- 5. HIGH SCORE & SAVE ---
                        if (gameData.score > gameData.highScore) {
                            gameData.highScore = gameData.score;
                            SaveHighScore();
                        }

                        audio->play("explosion.mp3");
                    }
                    else { audio->play("hit.mp3"); }
                    break;
                }
            }

            if (!spawnedMinions.empty()) {
                enemies.insert(enemies.end(), spawnedMinions.begin(), spawnedMinions.end());
            }
        }  

        // --- COLLISION: PLAYER vs POWERUP ---
        for (auto& p : powerups) {
            if (!p.active) continue;
            if (glm::distance(p.position, playerPos) < 1.5f) {
                p.active = false;
                if (p.type == POWER_LIFE) { gameData.lives++; }
                else if (p.type == POWER_TRIPLE) { gameData.tripleShotTimer = 5.0f; }
                else if (p.type == POWER_SHIELD) { immunityTimer = 5.0f; }
                else if (p.type == POWER_PIERCING) {
                    gameData.piercingTimer = 5.0f;
                }
            }
        }

        // --- COLLISION: PLAYER vs ENEMY ---
        for (int i = 0; i < enemies.size(); i++) {
            float hitboxSize = 1.5f; // Για μικρούς
            if (enemies[i].type == ENEMY_BOSS_MINE || enemies[i].type == ENEMY_BOSS_SHIP) {
                hitboxSize = 4.0f; // Για Bosses (Μεγαλύτερο κουτί)
            }
            if (abs(enemies[i].position.y - playerPos.y) < hitboxSize &&
                abs(enemies[i].position.x - playerPos.x) < hitboxSize) {
                if (immunityTimer <= 0.0f) {
                    gameData.lives--;
                    gameData.comboCount = 0;
                    gameData.multiplier = 1.0f;
                    gameData.comboTimer = 0.0f;
                    audio->play("explosion.mp3");
                    SpawnExplosion(playerPos);
                    if (gameData.lives > 0) {
                        immunityTimer = 2.0f;
                        enemies.erase(enemies.begin() + i); i--;
                    }
                    else {
                        gameData.state = GAME_OVER;
                        audio->play("gameover.mp3");
                    }
                }
            }
        }

        for (auto& b : enemyBullets) {
            if (!b.active) continue;
            // Αν μας πετύχει (απόσταση < 1.0)
            if (glm::distance(b.position, playerPos) < 1.0f) {
                b.active = false;

                if (immunityTimer <= 0.0f) {
                    gameData.lives--;
                    audio->play("explosion.mp3");
                    SpawnExplosion(playerPos);

                    if (gameData.lives > 0) {
                        immunityTimer = 2.0f;
                    }
                    else {
                        gameData.state = GAME_OVER;
                        audio->play("gameover.mp3");
                    }
                }
            }
        }

        // Cleanups
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](Bullet& b) { return b.position.y > 20.0f || !b.active; }), bullets.end());
        powerups.erase(std::remove_if(powerups.begin(), powerups.end(), [](PowerUp& p) { return p.position.y < -12.0f || !p.active; }), powerups.end());
        enemyBullets.erase(std::remove_if(enemyBullets.begin(), enemyBullets.end(), [](Bullet& b) { return b.position.y < -12.0f || !b.active; }), enemyBullets.end());
        // O Manager κανει το cleanup των enemies μονος του στο Update()
    }
}

void Game::SpawnExplosion(glm::vec3 position) {
    for (int i = 0; i < 20; i++) {
        Particle p; p.position = position;
        p.velocity = glm::vec3(randomFloat(-5.0f, 5.0f), randomFloat(-5.0f, 5.0f), 0.0f);
        p.color = glm::vec3(1.0f, randomFloat(0.5f, 1.0f), 0.0f); p.life = 1.0f;
        particles.push_back(p);
    }
}

void Game::Render() {
    shader->use();
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)Width / Height, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 18), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    shader->setMat4("projection", proj); shader->setMat4("view", view);
    glBindVertexArray(VAO);
    shader->setBool("flash", false);

    // Background
    texSpace->bind(); shader->setBool("useTexture", true);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, bgOffset, -5.0f));
    model = glm::scale(model, glm::vec3(30.0f, 20.0f, 1.0f));
    shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, bgOffset + 20.0f, -5.0f));
    model = glm::scale(model, glm::vec3(30.0f, 20.0f, 1.0f));
    shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);

    if (gameData.state != GAME_MENU && gameData.state != GAME_OVER) {
        // Player
        if (immunityTimer <= 0.0f || (int)(immunityTimer * 15.0f) % 2 == 0) {
            texShip->bind(); shader->setBool("useTexture", true);
            model = glm::translate(glm::mat4(1.0f), playerPos);
            model = glm::scale(model, glm::vec3(3.0f, 3.0f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // --- RENDER ENEMIES VIA MANAGER ---
        enemyManager->Render(shader);

        // PowerUps
        for (auto& p : powerups) {
            if (p.type == POWER_LIFE) { texShip->bind(); } // Life = Ship icon
            else if (p.type == POWER_TRIPLE) { texPowTriple->bind(); }
			else if (p.type == POWER_PIERCING) { texPowPiercing->bind(); }
            else { texPowShield->bind(); }

            model = glm::translate(glm::mat4(1.0f), p.position);
            if (p.type == POWER_LIFE) model = glm::scale(model, glm::vec3(2.0f, 2.0f, 1.0f));
            else model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.0f));

            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Bullets
        texBullet->bind(); shader->setBool("useTexture", true);
        for (auto& b : bullets) {
            model = glm::translate(glm::mat4(1.0f), b.position);
            model = glm::scale(model, glm::vec3(0.6f, 0.6f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // RENDER ENEMY BULLETS
        texBullet->bind();
        shader->setBool("useTexture", true);
        // Αν θες να φαίνονται διαφορετικές (π.χ. λίγο πιο κόκκινες με τον shader, ή απλά ίδιες)

        for (auto& b : enemyBullets) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), b.position);
            // Τις κάνουμε λίγο πιο μικρές
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 1.0f));
            shader->setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Particles
        shader->setBool("useTexture", false);
        for (auto& p : particles) {
            shader->setVec3("myColor", p.color.x, p.color.y, p.color.z);
            model = glm::translate(glm::mat4(1.0f), p.position);
            float size = p.life * 0.4f;
            model = glm::scale(model, glm::vec3(size, size, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // --- BOSS HP BAR ---
        // Βρίσκουμε αν υπάρχει Boss
        std::vector<Enemy>& enemies = enemyManager->GetEnemies();
        for (const auto& e : enemies) {
            if (e.type == ENEMY_BOSS_MINE || e.type == ENEMY_BOSS_SHIP) {
                // Υπολογισμός ποσοστού ζωής (0.0 έως 1.0)
                float hpPercent = (float)e.hp / (float)e.maxHp;

                // 1. Φόντο Μπάρας (Κόκκινο)
                shader->setBool("useTexture", false);
                shader->setVec3("myColor", 0.5f, 0.0f, 0.0f); // Σκούρο Κόκκινο

                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 9.0f, 0.0f)); // Ψηλά
                model = glm::scale(model, glm::vec3(10.0f, 0.5f, 1.0f)); // Πλάτος 10
                shader->setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                // 2. Γέμισμα Μπάρας (Πράσινο)
                shader->setVec3("myColor", 0.0f, 1.0f, 0.0f); // Πράσινο

                // Το scale στον αξονα X είναι το ποσοστό ζωής!
                // Μετακινουμε λιγο αριστερα για να γεμιζει απο αριστερα (απλοποιημενο κεντρικο γεμισμα εδω)
                model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 9.0f, 0.1f)); // Z=0.1 για να είναι μπροστά
                model = glm::scale(model, glm::vec3(10.0f * hpPercent, 0.5f, 1.0f));
                shader->setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // UI: Lives
        texShip->bind(); shader->setBool("useTexture", true);
        shader->setBool("flash", false);
        for (int i = 0; i < gameData.lives; i++) {
            float xPos = 5.5f + (i * 1.2f);
            model = glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 8.0f, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // UI: Indicators
        if (gameData.tripleShotTimer > 0.0f) {
            texPowTriple->bind();
            model = glm::translate(glm::mat4(1.0f), glm::vec3(7.0f, 6.5f, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        if (immunityTimer > 2.0f) {
            texPowShield->bind();
            model = glm::translate(glm::mat4(1.0f), glm::vec3(5.5f, 6.5f, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    glEnable(GL_BLEND);
    if (gameData.state == GAME_MENU) {
        texStart->bind(); shader->setBool("useTexture", true);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f)); model = glm::scale(model, glm::vec3(12.0f, 6.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else if (gameData.state == GAME_PAUSED) {
        texPause->bind(); shader->setBool("useTexture", true);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f)); model = glm::scale(model, glm::vec3(8.0f, 4.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else if (gameData.state == GAME_OVER) {
        texOver->bind(); shader->setBool("useTexture", true);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f)); model = glm::scale(model, glm::vec3(10.0f, 5.0f, 1.0f));
        shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void Game::LoadHighScore() {
    std::ifstream inputFile("highscore.txt"); // Προσπάθησε να ανοίξεις το αρχείο
    if (inputFile.is_open()) {
        inputFile >> gameData.highScore; // Διάβασε τον αριθμό
        inputFile.close();
        std::cout << "High Score Loaded: " << gameData.highScore << std::endl;
    }
    else {
        gameData.highScore = 0; // Αν δεν υπάρχει αρχείο (πρώτη φορά), βάλε 0
    }
}

void Game::SaveHighScore() {
    std::ofstream outputFile("highscore.txt"); // Άνοιξε για γράψιμο (θα διαγράψει το παλιό)
    if (outputFile.is_open()) {
        outputFile << gameData.highScore; // Γράψε το νέο ρεκόρ
        outputFile.close();
    }
}