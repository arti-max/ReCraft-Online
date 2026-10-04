#include "gamemode/GameMode.hpp"
#include "level/Level.hpp"
#include "level/tile/Tile.hpp"
#include "CrossCraft.hpp"
#include "net/packet/BlockChangePacket.hpp"
#include "player/Player.hpp"

GameMode::GameMode(CrossCraft* cc) {
    this->cc = cc;
    this->instantBreak = false;
}

void GameMode::apply(Level* level) {
    level->growTrees = true;
}

void GameMode::openInventory() {
    // nothing
}

void GameMode::hitTile(int x, int y, int z) {
    this->breakTile(x, y, z);
}

bool GameMode::canPlace(int tile) {
    return true;
}

void GameMode::breakTile(int x, int y, int z) {
    Level* level = this->cc->level;
    Tile* tile = Tile::tiles[level->getTile(x, y, z)];

    bool placed = level->setTile(x, y, z, 0);

    if (tile != nullptr && placed) {
        if (tile->st != &SoundType::none) {
            level->playSound("step." + tile->st->name, (float)x, (float)y, (float)z, (tile->st->getVolume() + 1.0f) / 2.0f, tile->st->getPitch() * 0.8f);
        }


        if (this->cc->mpMode) {
            BlockChangePacket* packet = new BlockChangePacket(x, y, z, 0);
            this->cc->client->sendPacket(packet);
        }
        
        tile->spawnDestroyParticles(level, x, y, z, cc->particleEngine);
    }
}

void GameMode::hitTile(int x, int y, int z, int face) {
    // nothing
}

void GameMode::resetHits() {
    // nothing
}

void GameMode::applyCracks(float time) {
    // nothing
}

bool GameMode::useItem(Player* player, int type) {
    return false;
}

void GameMode::preparePlayer(Player* player) {
}

void GameMode::spawnMob() {
    // nothing  
}

void GameMode::prepareLevel(Level* level) {
    ((Player*)level->getPlayer())->score = 0;
}

bool GameMode::isSurvival() {
    return true;
}

void GameMode::apply(Player* player) {
    // nothing
}

float GameMode::getReachDistance() {
    return 5.0f;
}