#pragma once
#include "mob/ai/BasicAttackAI.hpp"

class Skeleton;
class Mob;
class Level;

class SkeletonAI : public BasicAttackAI {
public:
    Skeleton* parent = nullptr;

    SkeletonAI(Skeleton* skeleton);
    ~SkeletonAI() override = default;
    void tick(Level* level, Mob* mob) override;
    void beforeRemove() override;
};