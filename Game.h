#pragma once
#ifndef GAME_H
#define GAME_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

// Forward Declarations (Gia na min kanoume include ta panta edw)
class Shader;
class Texture;
class Audio;

// --- ENUMS & STRUCTS ---
enum GameState { GAME_MENU, GAME_ACTIVE, GAME_PAUSED, GAME_OVER };

struct Bullet {
    glm::vec3 position;
    bool active;
};

struct Enemy {
    glm::vec3 position;
    float originalX;
    float speed;
    float wobbleOffset;
    int hp;           // Πόσες σφαίρες αντέχει
    float flashTimer;
};

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity; // Προς τα πού πάει
    glm::vec3 color;
    float life;         // Πόσο χρόνο ζωής έχει (π.χ. 1.0 δευτερόλεπτο)
};

struct GameData {
    int score;
    int highScore;
    int level;
    int lives; // <--- ΝΕΟ: Ζωές παίκτη
    GameState state;

    // Ξεκιναμε με 3 ζωες
    GameData() : score(0), highScore(0), level(1), lives(3), state(GAME_MENU) {}
};

// --- THE GAME CLASS ---
class Game {
public:
    // Constructor / Destructor
    Game(unsigned int width, unsigned int height);
    ~Game();

    GameData gameData;

    // Vasikes Leitourgies
    void Init();
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();

    // Input Callbacks
    void SetKeys(int key, bool pressed);
    void ProcessEvents(int key); // Gia Enter, R, P
    bool Keys[1024];
    unsigned int Width, Height;


private:
    // Resources (Pointers gia na ta diaxeirizomaste emeis)
    Shader* shader;
    Texture* texShip, * texAlien, * texSpace, * texStart, * texOver, * texPause, * texBullet, * texAlienHit;
    Audio* audio;

    // Game Objects
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    std::vector<Particle> particles;
    void SpawnExplosion(glm::vec3 position); // Συνάρτηση που δημιουργεί την έκρηξη
    glm::vec3 playerPos;

    // OpenGL Buffers
    unsigned int VAO, VBO;

    // Gameplay Variables
    float bgOffset;
    float lastSpawnTime;
    float currentSpawnInterval;
    float lastShootTime;
    float baseEnemySpeed;
    float immunityTimer;


    // Private Helpers
    void SpawnEnemy();
    void ResetLevel();
};

#endif