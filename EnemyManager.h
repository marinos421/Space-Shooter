#pragma once
#ifndef ENEMYMANAGER_H
#define ENEMYMANAGER_H

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

// ΑΛΛΑΓΗ: INCLUDE TO COMMONS ANTI GIA TO GAME
#include "Commons.h" 

// Forward Declarations (gia na min kanoume include ta panta)
class Shader;
class Texture;

class EnemyManager {
public:
    EnemyManager();
    ~EnemyManager();

    void Init();
    void Update(float dt, float currentTime, int level, glm::vec3 playerPos, std::vector<Bullet>& enemyBullets);
    void Render(Shader* shader);

    void SpawnEnemy(int level);
    std::vector<Enemy>& GetEnemies();
    void Clear();

private:
    std::vector<Enemy> enemies;

    Texture* texBasic, * texShooter, * texBoss, * texHit, * texShooterHit;
    Texture* texBossMine; // Boss Lvl 5 Assets
    Texture* texBossShip; // Boss Lvl 10 Assets

    float CalculateSpeed(int level);
    int CalculateHP(int level);

    float lastSpawnTime;
    float currentSpawnInterval;

    bool bossActive;

    float randomFloat(float min, float max);

    void SpawnBossMine();
    void SpawnBossShip();

    // Αυτές θα καλούνται μέσα από την Update
    void UpdateBossMine(Enemy& boss, float dt, glm::vec3 playerPos, std::vector<Bullet>& enemyBullets, std::vector<Enemy>& newEnemies);

    // Κάνε το ίδιο και στο Ship Boss για συμβατότητα
    void UpdateBossShip(Enemy& boss, float dt, glm::vec3 playerPos, std::vector<Bullet>& enemyBullets, std::vector<Enemy>& newEnemies);
};

#endif