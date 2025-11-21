#pragma once
#pragma once
#ifndef COMMONS_H
#define COMMONS_H

#include <glm/glm.hpp>
#include <vector>

// --- ENUMS ---
enum GameState { GAME_MENU, GAME_ACTIVE, GAME_PAUSED, GAME_OVER };

enum EnemyType {
    ENEMY_BASIC,
    ENEMY_SHOOTER,
    ENEMY_BOSS_MINE, // Boss Level 5 (Dive + Minions)
    ENEMY_BOSS_SHIP  // Boss Level 10 (Targeted + Spread)
};

// --- STRUCTS ---
struct Bullet {
    glm::vec3 position;
    glm::vec3 velocity;
    bool active;
};

struct Enemy {
    glm::vec3 position;
    float originalX;
    float speed;
    float wobbleOffset;
    int hp;
    int maxHp;
    float flashTimer;

    EnemyType type;
    float weaponTimer;

    // --- BOSS SPECIFIC FIELDS ---
    int phase;         // Φάση 1 ή 2
    int state;         // Τι κάνει τώρα; (π.χ. 0=Idle, 1=Diving, 2=Returning)
    float stateTimer;  // Χρονόμετρο για την τρέχουσα κίνηση (π.χ. πόση ώρα κάνει Dive)
    glm::vec3 targetPos;

    float minionTimer;
};

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float life;
};

struct GameData {
    int score;
    int highScore;
    int level;
    int lives;
    GameState state;
    float tripleShotTimer;

    GameData() : score(0), highScore(0), level(1), lives(3), state(GAME_MENU), tripleShotTimer(0.0f) {}
};

#endif