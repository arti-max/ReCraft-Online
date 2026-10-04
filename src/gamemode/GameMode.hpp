#pragma once
#include "sound/SoundType.hpp"

#define GM_CREATIVE 1
#define GM_SURVIVAL 0

class Player;
class Level;
class CrossCraft;
class Tile;

class GameMode {
public:
    bool instantBreak = false;
    int gmType = 1;
    CrossCraft* cc;

    GameMode(CrossCraft* cc);
    virtual void apply(Level* level);
    virtual void openInventory();
    virtual void hitTile(int x, int y, int z);
    virtual bool canPlace(int tile);
    virtual void breakTile(int x, int y, int z);
    virtual void hitTile(int x, int y, int z, int face);
    virtual void resetHits();
    virtual void applyCracks(float time);
    virtual float getReachDistance();
    virtual bool useItem(Player* player, int type);
    virtual void preparePlayer(Player* player);
    virtual void spawnMob();
    virtual void prepareLevel(Level* level);
    virtual bool isSurvival();
    virtual void apply(Player* player);
};