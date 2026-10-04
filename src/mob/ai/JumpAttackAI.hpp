#pragma once
#include "mob/ai/BasicAttackAI.hpp"


class JumpAttackAI : public BasicAttackAI {
public:
    JumpAttackAI();
    ~JumpAttackAI() override = default;
    void jumpFromGround() override;
};