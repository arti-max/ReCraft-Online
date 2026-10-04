#include "level/tile/SlabTile.hpp"

SlabTile::SlabTile(int tileId, int textureId) : Tile(tileId, textureId) {
    this->setShape(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
}

SlabTile::SlabTile(int tileId, bool isDouble) : Tile(tileId) {
    this->textureId = 6;
    this->doubleSlab = isDouble;
    if (!isDouble) {
        this->setShape(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    }
}

bool SlabTile::shouldRenderFace(Level* level, int x, int y, int z, int face) {
    if (face == 1 && !this->doubleSlab) return true;
    return Tile::shouldRenderFace(level, x, y, z, face);
}

void SlabTile::onBlockAdded(Level* level, int x, int y, int z) {
    if (this->id == Tile::slab->id) {
        int belowId = level->getTile(x, y-1, z);
        if (belowId == Tile::slab->id) {
            level->setTile(x, y-1, z, Tile::doubleSlab->id);
            level->setTile(x, y, z, 0);
        }
    }
}

int SlabTile::getDrop() {
    return Tile::slab->id;
}

int SlabTile::getDropCount() {
    if (this->doubleSlab) return 2;
    return 1;
}

int SlabTile::getTexture(int face) {
    return face <= 1 ? 6 : 5;
}

bool SlabTile::isSolid() {
    return this->doubleSlab;
}