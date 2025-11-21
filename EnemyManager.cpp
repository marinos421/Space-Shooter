#include "EnemyManager.h"

// Προσθήκη αυτών για να ξέρει τι είναι Texture και Shader
#include "Texture.h"
#include "Shader.h"

// Προσθήκη GLM για translate/scale
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <iostream>
#include <cmath>

EnemyManager::EnemyManager() : lastSpawnTime(0.0f), currentSpawnInterval(1.5f) { }

EnemyManager::~EnemyManager() {
    delete texBasic;
    delete texShooter;
    delete texBoss;
    delete texHit;
    delete texShooter;
    delete texShooterHit;
    delete texBossMine;
    delete texBossShip;
}

void EnemyManager::Init() {
    // Φορτώνουμε τα textures εδώ
    texBasic = new Texture("spacemine.png"); // Η παλιά alien.jpg/png
    texBoss = new Texture("alien_boss.png");       // Θα το βάλουμε μετά
    texHit = new Texture("minehit.png");
    texShooter = new Texture("enemyship.png");
    texShooterHit = new Texture("enemyship_hit.png");
    texBossMine = new Texture("boss_mine.png");
    texBossShip = new Texture("boss_ship.png");

    bossActive = false;
}

void EnemyManager::Clear() {
    enemies.clear();
    lastSpawnTime = 0.0f;
}

std::vector<Enemy>& EnemyManager::GetEnemies() {
    return enemies;
}

float EnemyManager::randomFloat(float min, float max) {
    return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

float EnemyManager::CalculateSpeed(int level) {
    // Η λογική που είχαμε στο Game.cpp
    return std::min(6.0f, 2.0f + (level * 0.2f));
}

int EnemyManager::CalculateHP(int level) {
    return 1 + (level / 3);
}

void EnemyManager::SpawnEnemy(int level) {

    // 1. ΕΛΕΓΧΟΣ ΓΙΑ BOSS (Κάθε 5 Levels)
    // Το (level % 5 == 0) σημαίνει: 5, 10, 15, 20, 25...
    if (level > 0 && level % 5 == 0) {

        if (!bossActive) {
            // Εναλλαγή Bosses
            // Αν το level είναι 5, 15, 25... (Περιττά πολλαπλάσια του 5) -> Mine Boss
            if (level % 10 == 5) {
                SpawnBossMine();
            }
            // Αν το level είναι 10, 20, 30... (Ζυγά πολλαπλάσια του 5) -> Ship Boss
            else {
                SpawnBossShip();
            }

            bossActive = true; // ΣΤΑΜΑΤΑΕΙ ΤΟΥΣ ΜΙΚΡΟΥΣ ΕΧΘΡΟΥΣ
            return; // ΦΕΥΓΟΥΜΕ ΑΠΟ ΤΗ ΣΥΝΑΡΤΗΣΗ
        }
    }

    // 2. ΑΝ ΥΠΑΡΧΕΙ BOSS, ΜΗΝ ΒΓΑΖΕΙΣ ΑΛΛΟΥΣ
    // (Τα minions τα βγάζει η UpdateBossMine μόνη της)
    if (bossActive) return;


    // --- 3. ΚΛΑΣΙΚΗ ΛΟΓΙΚΗ ΓΙΑ ΜΙΚΡΟΥΣ ΕΧΘΡΟΥΣ (Level 1-4, 6-9, κλπ) ---
    float randomX = randomFloat(-7.5f, 7.5f);
    Enemy e;
    e.position = glm::vec3(randomX, 12.0f, 0.0f);
    e.originalX = randomX;
    e.speed = CalculateSpeed(level);
    e.wobbleOffset = randomFloat(0.0f, 3.14f);

    e.hp = CalculateHP(level);
    e.flashTimer = 0.0f;
    e.weaponTimer = randomFloat(0.5f, 2.0f);

    e.type = ENEMY_BASIC;

    // Λογική για Shooter Enemies (30% πιθανότητα μετά το Level 3)
    // (Εξαιρούμε τα Boss Levels για σιγουριά)
    if (level >= 3 && level % 5 != 0) {
        if (rand() % 100 < 30) {
            e.type = ENEMY_SHOOTER;
            e.speed *= 0.7f;
        }
    }

    enemies.push_back(e);
}

// --- 2. Η ΝΕΑ UPDATE (O "ΤΡΟΧΟΝΟΜΟΣ") ---
void EnemyManager::Update(float dt, float currentTime, int level, glm::vec3 playerPos, std::vector<Bullet>& enemyBullets) {

    // 1. SPAWNER LOGIC
    currentSpawnInterval = std::max(0.5f, 1.5f - (level * 0.1f));
    if (currentTime - lastSpawnTime >= currentSpawnInterval) {
        SpawnEnemy(level);
        lastSpawnTime = currentTime;
    }

    // --- ΦΤΙΑΧΝΟΥΜΕ ΤΗΝ ΠΡΟΣΩΡΙΝΗ ΛΙΣΤΑ ---
    std::vector<Enemy> newEnemies;
    // -------------------------------------

    // 2. MOVEMENT & LOGIC LOOP
    for (auto& e : enemies) {
        if (e.flashTimer > 0.0f) e.flashTimer -= dt;

        if (e.type == ENEMY_BOSS_MINE) {
            // ΠΕΡΝΑΜΕ ΤΗ ΛΙΣΤΑ newEnemies
            UpdateBossMine(e, dt, playerPos, enemyBullets, newEnemies);
        }
        else if (e.type == ENEMY_BOSS_SHIP) {
            UpdateBossShip(e, dt, playerPos, enemyBullets, newEnemies);
        }
        else {
            // Βασική κίνηση για τους άλλους
            e.position.y -= e.speed * dt;

            if (level == 1) { e.position.x = e.originalX; }
            else {
                float intensity = std::min(3.0f, 1.0f + (level * 0.2f));
                float frequency = std::min(4.0f, 2.0f + (level * 0.1f));
                e.position.x = e.originalX + sin(currentTime * frequency + e.wobbleOffset) * intensity;
                if (e.position.x > 8.0f) e.position.x = 8.0f; if (e.position.x < -8.0f) e.position.x = -8.0f;
            }

            if (e.type == ENEMY_SHOOTER) {
                e.weaponTimer -= dt;
                if (e.weaponTimer <= 0.0f) {
                    Bullet b;
                    b.position = e.position - glm::vec3(0.0f, 1.0f, 0.0f);
                    b.velocity = glm::vec3(0.0f, -10.0f, 0.0f);
                    b.active = true;
                    enemyBullets.push_back(b);
                    e.weaponTimer = 2.5f;
                }
            }
        }
    }

    // --- ΣΥΓΧΩΝΕΥΣΗ: Προσθέτουμε τα minions στην κανονική λίστα ---
    if (!newEnemies.empty()) {
        enemies.insert(enemies.end(), newEnemies.begin(), newEnemies.end());
    }

    // 3. CLEANUP (ΔΙΟΡΘΩΜΕΝΟ)
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](Enemy& e) {
            // ΚΑΝΟΝΑΣ 1: Αν η θέση είναι πολύ χαμηλά (π.χ. -50), σημαίνει ότι ΠΕΘΑΝΕ.
            // Άρα ΣΒΗΣΤΟ (return true), ακόμα κι αν είναι Boss!
            if (e.position.y <= -40.0f) return true;

            // ΚΑΝΟΝΑΣ 2: Αν είναι Boss και δεν έχει πεθάνει (είναι ακόμα στην οθόνη ή κάνει βουτιά),
            // ΚΡΑΤΑ ΤΟ (return false).
            if (e.type == ENEMY_BOSS_MINE || e.type == ENEMY_BOSS_SHIP) return false;

            // ΚΑΝΟΝΑΣ 3: Για τους απλούς εχθρούς, αν βγουν κάτω από την οθόνη (-12),
            // ΣΒΗΣΤΟΥΣ (return true).
            return e.position.y < -12.0f;
        }), enemies.end());

    // --- ΕΛΕΓΧΟΣ ΑΝ ΠΕΘΑΝΕ ΤΟ BOSS ---
    if (bossActive) {
        bool bossFound = false;
        for (const auto& e : enemies) {
            if (e.type == ENEMY_BOSS_MINE || e.type == ENEMY_BOSS_SHIP) {
                bossFound = true;
                break;
            }
        }
        // Αν το bossActive είναι true, αλλά δεν υπάρχει Boss στη λίστα -> ΠΕΘΑΝΕ
        if (!bossFound) {
            bossActive = false; // Ξεμπλοκάρουμε το spawn των εχθρών!
        }
    }
}

void EnemyManager::Render(Shader* shader) {
    for (auto& e : enemies) {

        bool useShaderFlash = false; // Από προεπιλογή κλειστό

        // --- BOSSES (Χρήση Shader Flash) ---
        if (e.type == ENEMY_BOSS_MINE) {
            texBossMine->bind(); // Πάντα η κανονική εικόνα
            if (e.flashTimer > 0.0f) useShaderFlash = true; // Άναψε τον shader
        }
        else if (e.type == ENEMY_BOSS_SHIP) {
            texBossShip->bind();
            if (e.flashTimer > 0.0f) useShaderFlash = true;
        }
        // --- ΜΙΚΡΟΙ ΕΧΘΡΟΙ (Χρήση Texture Swap) ---
        else {
            // Αν χτυπήθηκαν, βάλε τη σπασμένη εικόνα
            if (e.flashTimer > 0.0f) {
                if (e.type == ENEMY_SHOOTER) texShooterHit->bind();
                else texHit->bind();

                // Εδώ το useShaderFlash μένει false, γιατί η ίδια η εικόνα είναι λευκή/σπασμένη
            }
            // Κανονική κατάσταση
            else {
                if (e.type == ENEMY_SHOOTER) texShooter->bind();
                else texBasic->bind();
            }
        }

        // Στέλνουμε την εντολή στον Shader
        shader->setBool("flash", useShaderFlash);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), e.position);

        // Scale ανάλογα με το αν είναι Boss ή όχι
        if (e.type == ENEMY_BOSS_MINE || e.type == ENEMY_BOSS_SHIP) {
            model = glm::scale(model, glm::vec3(5.0f, 5.0f, 1.0f)); // Μεγάλο Boss
        }
        else {
            model = glm::scale(model, glm::vec3(2.0f, 2.0f, 1.0f)); // Μικρός Εχθρός
        }

        shader->setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void EnemyManager::SpawnBossMine() {
    Enemy boss;
    // Ξεκινάει ψηλά στο κέντρο
    boss.position = glm::vec3(0.0f, 12.0f, 0.0f);
    boss.originalX = 0.0f;

    // Stats
    boss.hp = 60; // Αρκετή ζωή
    boss.maxHp = 60;
    boss.speed = 3.0f;

    boss.type = ENEMY_BOSS_MINE;
    boss.flashTimer = 0.0f;

    // State Machine Variables
    boss.state = 0;       // 0 = Hovering, 1 = Diving, 2 = Returning
    boss.stateTimer = 0.0f;
    boss.weaponTimer = 3.0f; // Χρόνος μέχρι την πρώτη βουτιά

    boss.minionTimer = 0.0f;
    boss.wobbleOffset = 0.0f;

    enemies.push_back(boss);
    bossActive = true; // Σημαντικό για να σταματήσουν οι άλλοι εχθροί
}

void EnemyManager::SpawnBossShip() {
    Enemy boss;
    boss.position = glm::vec3(0.0f, 12.0f, 0.0f); // Ξεκινάει ψηλά
    boss.originalX = 0.0f;

    // STATS (Πιο σκληρό από το Mine Boss)
    boss.hp = 100;
    boss.maxHp = 100;

    boss.speed = 2.0f; // Αργό και επιβλητικό
    boss.type = ENEMY_BOSS_SHIP;
    boss.flashTimer = 0.0f;

    // State Machine
    boss.state = 0;
    boss.stateTimer = 0.0f;
    boss.weaponTimer = 2.0f; // Πρώτη βολή σε 2 δευτερόλεπτα
    boss.minionTimer = 0.0f; // Δεν βγάζει minions, αλλά το αρχικοποιούμε
    boss.wobbleOffset = 0.0f;

    enemies.push_back(boss);
    bossActive = true;
}

void EnemyManager::UpdateBossMine(Enemy& boss, float dt, glm::vec3 playerPos, std::vector<Bullet>& enemyBullets, std::vector<Enemy>& newEnemies) {

    // STATE 0: HOVER (Κίνηση δεξιά-αριστερά ψηλά)
    if (boss.state == 0) {
        // Εισαγωγή: Αν είναι πολύ ψηλά, κατέβασε το λίγο
        if (boss.position.y > 7.0f) {
            boss.position.y -= dt * 2.0f;
        }

        // Κίνηση Ημίτονου
        boss.stateTimer += dt;
        boss.position.x = sin(boss.stateTimer) * 6.0f;

        // Αντίστροφη μέτρηση για επίθεση (Dive)
        boss.weaponTimer -= dt;
        if (boss.weaponTimer <= 0.0f) {
            boss.state = 1; // Αλλαγή σε Dive
            boss.targetPos = playerPos; // Σημάδεψε πού ήταν ο παίκτης
        }
    }
    // STATE 1: DIVE (Επίθεση προς τα κάτω)
    else if (boss.state == 1) {
        // Κινήσου γρήγορα προς το στόχο (μόνο στον άξονα Χ για διόρθωση, και φουλ κάτω στον Y)
        float speed = 15.0f;
        boss.position.y -= speed * dt;

        // Αν φτάσει χαμηλά, γύρνα πίσω
        if (boss.position.y <= -5.0f) {
            boss.state = 2; // Return
        }
    }
    // STATE 2: RETURN (Επιστροφή ψηλά)
    else if (boss.state == 2) {
        float speed = 5.0f;
        boss.position.y += speed * dt; // Πήγαινε πάνω

        // Μετακινήσου αργά προς το κέντρο (0)
        if (boss.position.x > 0.1f) boss.position.x -= speed * dt;
        if (boss.position.x < -0.1f) boss.position.x += speed * dt;

        // Αν έφτασε ψηλά, ξανά Hover
        if (boss.position.y >= 7.0f) {
            boss.state = 0;
            boss.weaponTimer = 3.0f; // Ξανά επίθεση σε 3 δευτερόλεπτα
        }
    }

    // --- PHASE 2: MINIONS (Όταν έχει κάτω από 50% ζωή) ---
    if (boss.hp < (boss.maxHp / 2)) {

        // ΕΛΕΓΧΟΣ: Βγάζουμε Minions ΜΟΝΟ όταν το Boss είναι στο State 0 (Hovering)
        // και όχι όταν κάνει βουτιά.
        if (boss.state == 0) {

            boss.minionTimer += dt;

            if (boss.minionTimer > 1.0f) { // Κάθε 2 δευτερόλεπτα

                // Δημιουργία Minion
                Enemy minion;
                // Το βάζουμε να βγαίνει λίγο πιο κάτω από το Boss για να φαίνεται
                minion.position = boss.position - glm::vec3(0.0f, 1.0f, 0.0f);

                // Κληρονομεί τη θέση Χ του Boss για να βγει από "μέσα" του
                minion.originalX = boss.position.x;

                minion.speed = 4.0f; // Πέφτουν γρήγορα
                minion.wobbleOffset = randomFloat(0.0f, 3.14f); // Τυχαία κίνηση

                minion.hp = 1;
                minion.maxHp = 1;
                minion.type = ENEMY_BASIC; // Φαίνεται σαν spacemine
                minion.flashTimer = 0.0f;
                minion.weaponTimer = 0.0f;

                // ΠΡΟΣΘΗΚΗ ΣΤΗ ΛΙΣΤΑ
                newEnemies.push_back(minion);

                boss.minionTimer = 0.0f; // Reset timer
            }
        }
    }
}

void EnemyManager::UpdateBossShip(Enemy& boss, float dt, glm::vec3 playerPos, std::vector<Bullet>& enemyBullets, std::vector<Enemy>& newEnemies) {

    // 1. ΚΙΝΗΣΗ (Hovering)
    // Κατεβαίνει στην αρχή μέχρι το Y=8.0
    if (boss.position.y > 8.0f) {
        boss.position.y -= dt * 1.5f;
    }

    // Κινείται δεξιά-αριστερά (Ημίτονο)
    boss.stateTimer += dt;
    boss.position.x = sin(boss.stateTimer * 0.8f) * 5.0f; // Αργή, πλατιά κίνηση

    // 2. ΕΠΙΘΕΣΗ
    boss.weaponTimer -= dt;
    if (boss.weaponTimer <= 0.0f) {

        // --- PHASE 1: TARGETED SHOTS (100% - 50% HP) ---
        if (boss.hp > (boss.maxHp / 2)) {
            // Υπολογισμός κατεύθυνσης προς τον παίκτη
            glm::vec3 direction = playerPos - boss.position;
            direction = glm::normalize(direction); // Την κάνουμε μονάδα (μήκος 1)

            // Ρίχνουμε 2 σφαίρες (αριστερά και δεξιά από το κέντρο του boss)
            Bullet b1;
            b1.position = boss.position - glm::vec3(1.5f, 1.0f, 0.0f);
            b1.velocity = direction * 12.0f; // Γρήγορη σφαίρα
            b1.active = true;
            enemyBullets.push_back(b1);

            Bullet b2;
            b2.position = boss.position - glm::vec3(-1.5f, 1.0f, 0.0f);
            b2.velocity = direction * 12.0f;
            b2.active = true;
            enemyBullets.push_back(b2);

            boss.weaponTimer = 1.2f; // Γρήγορος ρυθμός βολής
        }

        // --- PHASE 2: BULLET FAN (KATO APO 50% HP) ---
        else {
            // Ρίχνει 5 σφαίρες σε σχήμα βεντάλιας
            // Χρειαζόμαστε λίγο τριγωνομετρία εδώ, αλλά θα το κάνουμε απλά με σταθερά vectors

            // Κεντρική
            Bullet b; b.active = true; b.position = boss.position - glm::vec3(0, 1, 0);
            b.velocity = glm::vec3(0.0f, -9.0f, 0.0f); // Κάτω
            enemyBullets.push_back(b);

            // Λίγο αριστερά
            b.velocity = glm::vec3(-3.0f, -8.5f, 0.0f); enemyBullets.push_back(b);
            // Πολύ αριστερά
            b.velocity = glm::vec3(-6.0f, -7.5f, 0.0f); enemyBullets.push_back(b);

            // Λίγο δεξιά
            b.velocity = glm::vec3(3.0f, -8.5f, 0.0f); enemyBullets.push_back(b);
            // Πολύ δεξιά
            b.velocity = glm::vec3(6.0f, -7.5f, 0.0f); enemyBullets.push_back(b);

            boss.weaponTimer = 1.8f; // Πιο αργός ρυθμός γιατί γεμίζει την οθόνη
        }
    }
}