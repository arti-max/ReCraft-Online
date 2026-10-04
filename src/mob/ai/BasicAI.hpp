#pragma once

#include "mob/ai/AI.hpp"
#include "util/Random.hpp"
#include <vector>

class Entity;
class Level;
class Mob;


class BasicAI : public AI {
public:
    Random random;
    float xxa = 0.0f; // Ax
    float yya = 0.0f; // Ay
    float yRotA = 0.0f;
    Level* level;
    Mob* mob;
    bool jumping = false;
    int attackDelay = 0;
    float runSpeed = 0.7f;
    int noActionTime = 0;
    Entity* attackTarget = nullptr;

    ~BasicAI() override = default;
    void tick(Level* level, Mob* mob) override;
    virtual void jumpFromGround();
    virtual void update();
    void beforeRemove() override;
    void hurt(Entity* e, int dmg) override;
};