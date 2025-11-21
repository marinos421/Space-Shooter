#pragma once
#ifndef GAME_H
#define GAME_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

// ΑΛΛΑΓΕΣ ΕΔΩ:
#include "Commons.h"      // <-- ΓΙΑ ΤΑ STRUCTS
#include "EnemyManager.h" // <-- ΓΙΑ ΤΟΝ MANAGER
#include "PowerUp.h" 

// Forward Declarations
class Shader;
class Texture;
class Audio;



class Game {
public:
    Game(unsigned int width, unsigned int height);
    ~Game();

    GameData gameData; // Τώρα το βρίσκει από το Commons.h

    void Init();
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();

    void SetKeys(int key, bool pressed);
    void ProcessEvents(int key);

    bool Keys[1024];
    unsigned int Width, Height;

private:
    Shader* shader;
    Texture* texShip, * texSpace, * texStart, * texOver, * texPause, * texBullet;
    Texture* texPowTriple, * texPowShield;

    Audio* audio;

    EnemyManager* enemyManager; // Τώρα το ξέρει από το EnemyManager.h

    std::vector<Bullet> bullets;      // Οι δικές σου
    std::vector<Bullet> enemyBullets;
    std::vector<Particle> particles;
    std::vector<PowerUp> powerups;

    void SpawnExplosion(glm::vec3 position);
    void SpawnPowerUp(glm::vec3 position);

    glm::vec3 playerPos;
    unsigned int VAO, VBO;

    float bgOffset;
    float lastShootTime;
    float immunityTimer;

    void ResetLevel();
};

#endif