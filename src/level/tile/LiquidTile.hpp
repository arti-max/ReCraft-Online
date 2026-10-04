#pragma once
#include "level/tile/Tile.hpp"

class LiquidTile : public Tile {
protected:
    LiquidType liquidType;
    int calmTileId = 0;
    int tileId = 0;
    int tickRate = 0;

    bool shouldRenderFace(Level* level, int x, int y, int z, int face) override;
private:
    bool tryFlow(Level* level, int x, int y, int z);
public:
    LiquidTile(int id, LiquidType liquidType);
    ~LiquidTile();
    void tick(Level* level, int x, int y, int z, Random* random) override;
    void renderFace(Tessellator& t, int x, int y, int z, int face) override;
    void neighborChanged(Level* level, int x, int y, int z, int type) override;
    bool mayPick() override;
    AABB* getAABB(int x, int y, int z) const override;
    bool blocksLight() override;
    bool isSolid() override;
    LiquidType getLiquidType() override;
    float getBrightness(Level* level, int x, int y, int z) override;
    short getRenderPass() override;
};