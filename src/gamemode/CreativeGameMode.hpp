#pragma once
#include "gamemode/GameMode.hpp"

class CreativeGameMode : public GameMode {
public:
    CreativeGameMode(CrossCraft* cc);
    void apply(Level* level) override;
    void openInventory() override;
    bool isSurvival() override;
    void apply(Player* player) override;
};