#include "gamemode/CreativeGameMode.hpp"
#include "CrossCraft.hpp"
#include "Data.hpp"
#include "Logger.hpp"
#include "gamemode/GameMode.hpp"
#include "gui/ingame/BlockSelectScreen.hpp"
#include "player/Player.hpp"

CreativeGameMode::CreativeGameMode(CrossCraft* cc) : GameMode(cc) {
    this->instantBreak = true;
    this->gmType = 1;
}

void CreativeGameMode::apply(Level* level) {
    GameMode::apply(level);

    level->growTrees = false;
}

void CreativeGameMode::openInventory() {
    BlockSelectScreen* screen = new BlockSelectScreen();
    this->cc->player->releaseAllKeys();
    this->cc->setScreen(screen);
    this->cc->releaseMouse();
}

bool CreativeGameMode::isSurvival() {
    return false;
}

void CreativeGameMode::apply(Player* player) {
    // Logger::logf(PREFIX_DEBUG, "Player: %i", (player != nullptr ? 1 : 0));
    for (int slot = 0; slot < 9; slot++) {
        player->inventory->count[slot] = 1;
        if (player->inventory->slots[slot] <= 0) {
            player->inventory->slots[slot] = ((Tile*)(Data::allowedTiles[slot]))->id;
        }
    }
}