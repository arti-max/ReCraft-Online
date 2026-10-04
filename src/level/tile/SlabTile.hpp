#pragma once
#include "level/tile/Tile.hpp"

class SlabTile : public Tile {
private:
    bool doubleSlab = false;
protected:
    bool shouldRenderFace(Level* level, int x, int y, int z, int face) override;
public:
    SlabTile(int tileId, int textureId);
    SlabTile(int tileId, bool isDouble);
    bool isSolid() override;
    void onBlockAdded(Level* level, int x, int y, int z) override;
    int getDrop() override;
    int getDropCount() override;
    int getTexture(int face) override;
};