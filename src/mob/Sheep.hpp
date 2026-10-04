#pragma once
#include "Entity.hpp"
#include "mob/Mob.hpp"

class Sheep : public Mob {
public:
    bool hasFur = true;
    bool grazing = false;
    int grazingTime = 0;
    float graze = 0.0f;
    float grazeO = 0.0f;

    Sheep(Level* level, float x, float y, float z);
    void aiStep() override;
    void die(Entity* e) override;
    void hurt(Entity* e, int dmg) override;
    void renderModel(Textures* textures, float time, float speed, float tick, float headYRot, float headXRot, float scale) override;
};