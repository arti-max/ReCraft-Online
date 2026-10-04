#pragma once
#include "player/Player.hpp"
#include "level/Chunk.hpp"

struct DistanceSorter {
    Player* player;
    int layer; // Добавляем слой
    
    DistanceSorter(Player* p, int l) : player(p), layer(l) {}
    
    bool operator()(Chunk* a, Chunk* b) const {
        if (layer == 1) {
            return a->distanceToSqr(player) > b->distanceToSqr(player);
        } else {
            return a->distanceToSqr(player) < b->distanceToSqr(player);
        }
    }
};
