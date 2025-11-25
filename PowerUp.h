#pragma once
#ifndef POWERUP_H
#define POWERUP_H

#include <glm/glm.hpp>

// Είδη Power-Ups
enum PowerUpType {
    POWER_LIFE,
    POWER_TRIPLE,
    POWER_SHIELD,
    POWER_PIERCING
};

struct PowerUp {
    glm::vec3 position;
    PowerUpType type;
    bool active;
    float velocityY; // Πέφτουν προς τα κάτω

    PowerUp(glm::vec3 pos, PowerUpType t)
        : position(pos), type(t), active(true), velocityY(2.0f) {}
};

#endif
