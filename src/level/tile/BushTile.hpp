#pragma once
#include "level/tile/Tile.hpp"
#include <cmath>

class Bush : public Tile {
public:
    Bush(int id, int textureId);
    void tick(Level* level, int x, int y, int z, Random* random) override;
    bool render(Tessellator& t, Level* level, int x, int y, int z) override;
    bool render(Tessellator& t, float x, float y, float z);
    AABB* getAABB(int x, int y, int z) const override;
    bool blocksLight() override;
    bool isSolid() override;
    void renderPreview(Tessellator& t) override;
};