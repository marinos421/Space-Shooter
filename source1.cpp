#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib> 
#include <ctime> 
#include <cmath>

// --- INCLUDES (Τα headers που εφτιαξες πριν) ---
#define STB_IMAGE_IMPLEMENTATION
#include "Shader.h"
#include "Texture.h"

using namespace std;

// --- CONFIGURATION ---
const int WINDOW_WIDTH = 850;
const int WINDOW_HEIGHT = 850;
const char* GAME_TITLE = "Space Shooter Refactored";

enum GameState { GAME_ACTIVE, GAME_PAUSED, GAME_OVER };

struct Bullet {
    glm::vec3 position;
    bool active;
};

struct Enemy {
    glm::vec3 position;
    float originalX;
    float speed;
    float wobbleOffset;
};

struct GameData {
    int score;
    int highScore;
    int level;
    GameState state;
    GameData() : score(0), highScore(0), level(1), state(GAME_ACTIVE) {}
};

// --- GLOBALS ---
GameData gameData;
glm::vec3 characterPosition = glm::vec3(0.0f, -8.0f, 0.0f);
std::vector<Enemy> enemies;
std::vector<Bullet> bullets;

// Settings
float moveStep = 12.0f;
float xLimit = 8.0f;
float baseEnemySpeed = 2.5f;
float currentSpawnInterval = 1.2f;
float lastSpawnTime = 0.0f;
float bulletSpeed = 18.0f;
float lastShootTime = 0.0f;
float shootCooldown = 0.2f;

bool keys[1024];

// --- HELPERS ---
void updateWindowTitle(GLFWwindow* window) {
    stringstream ss;
    ss << GAME_TITLE << " | LVL: " << gameData.level
        << " | Score: " << gameData.score
        << " | High Score: " << gameData.highScore;
    if (gameData.state == GAME_PAUSED) ss << " [PAUSED]";
    if (gameData.state == GAME_OVER) ss << " [GAME OVER - Press R]";
    glfwSetWindowTitle(window, ss.str().c_str());
}

void resetGame(GLFWwindow* window) {
    gameData.score = 0; gameData.level = 1; gameData.state = GAME_ACTIVE;

    // Reset Difficulty (Level 1 values)
    baseEnemySpeed = 2.0f; // Ξεκιναμε λιγο πιο αργα
    currentSpawnInterval = 1.5f; // Ξεκιναμε πιο αραια

    characterPosition = glm::vec3(0.0f, -8.0f, 0.0f);
    enemies.clear(); bullets.clear();
    updateWindowTitle(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keys[key] = true; else if (action == GLFW_RELEASE) keys[key] = false;
    }
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if ((gameData.state == GAME_OVER || gameData.state == GAME_PAUSED) && key == GLFW_KEY_R && action == GLFW_PRESS) resetGame(window);
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (gameData.state == GAME_ACTIVE) { gameData.state = GAME_PAUSED; updateWindowTitle(window); }
        else if (gameData.state == GAME_PAUSED) { gameData.state = GAME_ACTIVE; lastSpawnTime = glfwGetTime(); updateWindowTitle(window); }
    }
}

float randomFloat(float min, float max) {
    return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

int main() {
    srand(static_cast <unsigned> (time(0)));
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GAME_TITLE, NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    glewExperimental = true; glewInit();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // Load Assets (Mesa apo tis classes)
    Shader* shader = new Shader("Basic.vert", "Basic.frag");
    Texture* texShip = new Texture("ship.png");
    Texture* texAlien = new Texture("alien.png");
    Texture* texOver = new Texture("gameover.png");
    Texture* texPause = new Texture("pause.png");

    float quadVertices[] = { -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f };
    unsigned int VAO, VBO; glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);

    float deltaTime = 0.0f, lastFrame = 0.0f;
    resetGame(window); // Arxikopoiisi difficulty

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (gameData.state == GAME_ACTIVE) {
            // Input
            if (keys[GLFW_KEY_L] && characterPosition.x < xLimit) characterPosition.x += moveStep * deltaTime;
            if (keys[GLFW_KEY_J] && characterPosition.x > -xLimit) characterPosition.x -= moveStep * deltaTime;

            if (keys[GLFW_KEY_SPACE] && currentFrame - lastShootTime >= shootCooldown) {
                Bullet b; b.position = characterPosition + glm::vec3(0, 1.5f, 0); b.active = true;
                bullets.push_back(b); lastShootTime = currentFrame;
            }

            // LEVEL UP SYSTEM
            int newLevel = 1 + (gameData.score / 500); // Kathe 500 pontous
            if (newLevel > gameData.level) {
                gameData.level = newLevel;
                // Auksisi Taxytitas kai Syxnotitas
                baseEnemySpeed = 2.0f + (gameData.level * 0.4f);
                currentSpawnInterval = max(0.4f, 1.5f - (gameData.level * 0.15f));
                updateWindowTitle(window);
            }

            // Spawner
            if (currentFrame - lastSpawnTime >= currentSpawnInterval) {
                float randomX = randomFloat(-7.5f, 7.5f);
                Enemy e;
                e.position = glm::vec3(randomX, 12.0f, 0.0f);
                e.originalX = randomX;
                e.speed = baseEnemySpeed;
                e.wobbleOffset = randomFloat(0.0f, 3.14f);
                enemies.push_back(e); lastSpawnTime = currentFrame;
            }

            // --- 4. UPDATES (Movement Logic) ---
            for (auto& b : bullets) if (b.active) b.position.y += bulletSpeed * deltaTime;

            for (auto& e : enemies) {
                // A. Vertical Movement (Kato)
                e.position.y -= e.speed * deltaTime;

                // B. Lateral Movement (Zig-Zag)
                if (gameData.level == 1) {
                    e.position.x = e.originalX;
                }
                else {
                    // VAZOUME TAVANI sto poso kounietai (mexri 3.5 monades deksia-aristera)
                    // Xrisimopoioume tin std::min gia na min paei sto apeiro
                    float intensity = std::min(3.5f, 1.0f + (gameData.level * 0.3f));
                    float frequency = 2.5f + (gameData.level * 0.1f);

                    float offsetX = sin(currentFrame * frequency + e.wobbleOffset) * intensity;
                    float newX = e.originalX + offsetX;

                    // CLAMPING: Kratame ton exthro mesa stin othoni (anamesa sto -8.5 kai 8.5)
                    if (newX > 8.5f) newX = 8.5f;
                    if (newX < -8.5f) newX = -8.5f;

                    e.position.x = newX;
                }
            }

            // Collisions
            bool scoreChanged = false;
            for (auto& b : bullets) {
                if (!b.active) continue;
                for (int i = 0; i < enemies.size(); i++) {
                    if (glm::distance(b.position, enemies[i].position) < 1.2f) {
                        b.active = false; enemies.erase(enemies.begin() + i);
                        gameData.score += 100; if (gameData.score > gameData.highScore) gameData.highScore = gameData.score;
                        scoreChanged = true; i--; break;
                    }
                }
            }
            if (scoreChanged) updateWindowTitle(window);

            for (auto& e : enemies) {
                if (abs(e.position.y - characterPosition.y) < 1.5f && abs(e.position.x - characterPosition.x) < 1.5f) {
                    gameData.state = GAME_OVER; updateWindowTitle(window);
                }
            }
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](Bullet& b) { return b.position.y > 20.0f || !b.active; }), bullets.end());
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](Enemy& e) { return e.position.y < -12.0f; }), enemies.end());
        }

        // --- RENDER ---
        if (gameData.state == GAME_OVER) glClearColor(0.3f, 0.0f, 0.0f, 1.0f);
        else if (gameData.state == GAME_PAUSED) glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        else glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 18), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        shader->setMat4("projection", proj);
        shader->setMat4("view", view);

        glBindVertexArray(VAO);

        if (gameData.state != GAME_OVER) {
            // Ship
            texShip->bind(); shader->setBool("useTexture", true);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), characterPosition);
            model = glm::scale(model, glm::vec3(3.0f, 3.0f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);

            // Enemies
            texAlien->bind();
            for (auto& e : enemies) {
                model = glm::translate(glm::mat4(1.0f), e.position);
                model = glm::scale(model, glm::vec3(2.0f, 2.0f, 1.0f));
                shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            // Bullets
            shader->setBool("useTexture", false); shader->setVec3("myColor", 1.0f, 0.8f, 0.0f);
            for (auto& b : bullets) {
                model = glm::translate(glm::mat4(1.0f), b.position);
                model = glm::scale(model, glm::vec3(0.3f, 0.8f, 1.0f));
                shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        glEnable(GL_BLEND);
        if (gameData.state == GAME_PAUSED) {
            texPause->bind(); shader->setBool("useTexture", true);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f)); model = glm::scale(model, glm::vec3(8.0f, 4.0f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else if (gameData.state == GAME_OVER) {
            texOver->bind(); shader->setBool("useTexture", true);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 5.0f)); model = glm::scale(model, glm::vec3(10.0f, 5.0f, 1.0f));
            shader->setMat4("model", model); glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glfwSwapBuffers(window); glfwPollEvents();
    }

    delete shader; delete texShip; delete texAlien; delete texOver; delete texPause;
    glfwTerminate(); return 0;
}