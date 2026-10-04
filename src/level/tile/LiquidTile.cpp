#include "level/tile/LiquidTile.hpp"
#include "level/liquid/LiquidType.hpp"

LiquidTile::LiquidTile(int id, LiquidType liquidType) : Tile::Tile(id) {
    this->liquidType = liquidType;
    this->textureId = 14;
    if (this->liquidType == LiquidType::LAVA) {
        this->textureId = 30;
    }

    if (this->liquidType == LiquidType::WATER) {
        this->tickRate = 40;
    } else {
        this->tickRate = 2;
    }

    this->tileId = id;
    this->calmTileId = id++;
    float dd = 0.1f;
    this->setShape(0.0f, 0.0f - dd, 0.0f, 1.0f, 1.0f - dd, 1.0f);
}

bool LiquidTile::tryFlow(Level* level, int x, int y, int z) {
    if (level->getTile(x, y, z) == 0 && this->liquidType == LiquidType::WATER) {
        for (int xx = x - 2; xx <= x + 2; xx++) {
            for (int yy = y - 2; yy <= y + 2; yy++) {
                for (int zz = z - 2; zz <= z + 2; zz++) {
                   int tileId = level->getTile(xx, yy, zz);
                   if (tileId == Tile::sponge->id) { 
                       return false;
                   }
                }
            }
        }

        if (level->setTile(x, y, z, this->tileId)) {
            level->addToTickNextTick(x, y, z, this->tileId);
        }
    } else if (level->getTile(x, y, z) == 0 && this->liquidType == LiquidType::LAVA) {
        if (level->setTile(x, y, z, this->tileId)) {
            level->addToTickNextTick(x, y, z, this->tileId);
        }
    }
    return false;
}

float LiquidTile::getBrightness(Level* level, int x, int y, int z) {
    return this->liquidType == LiquidType::LAVA ? 100.0f : level->getBrightness(x, y, z);
}

void LiquidTile::tick(Level* level, int x, int y, int z, Random* random) {
    bool hasChanged = false;
    int originalY = y;

    bool flowedDown;
    do {
        y--;
        if (level->getTile(x, y, z) != 0) break;
        
        flowedDown = level->setTile(x, y, z, this->tileId);
        if (flowedDown) hasChanged = true;
        
    } while (flowedDown && this->liquidType != LiquidType::LAVA);
    
    y = originalY;

    if (this->liquidType == LiquidType::WATER || !hasChanged) {
        tryFlow(level, x - 1, y, z);
        tryFlow(level, x + 1, y, z);
        tryFlow(level, x, y, z - 1);
        tryFlow(level, x, y, z + 1);
    }
}



bool LiquidTile::shouldRenderFace(Level* level, int x, int y, int z, int face) {
    int neighborId = level->getTile(x, y, z);

    if (Tile::tiles[neighborId] && Tile::tiles[neighborId]->getLiquidType() != LiquidType::NOT_LIQUID) {
        return false;
    }
    
    if (neighborId != this->tileId && neighborId != this->calmTileId) {
        return face != 1 || level->getTile(x-1, y, z) != 0 && level->getTile(x+1, y, z != 0) && level->getTile(x, y, z-1) != 0 && level->getTile(x, y, z+1) != 0 ? Tile::shouldRenderFace(level, x, y, z, face) : true;
    }
    
    return false;
}

void LiquidTile::renderFace(Tessellator& t, int x, int y, int z, int face) {
    Tile::renderFace(t, x, y, z, face);
    
    if (face == 0 || face == 1) {
        Tile::renderBackFace(t, x, y, z, face);
    }
}

bool LiquidTile::mayPick() {
    return false;
}

AABB* LiquidTile::getAABB(int x, int y, int z) const {
    return nullptr;
}

bool LiquidTile::blocksLight() {
    return true;
}

bool LiquidTile::isSolid() {
    return false;
}

LiquidType LiquidTile::getLiquidType() {
    return this->liquidType;
}

void LiquidTile::neighborChanged(Level* level, int x, int y, int z, int neighborTileId) {
    if (this->liquidType == LiquidType::WATER && (neighborTileId == Tile::lava->id || neighborTileId == Tile::calmLava->id)) {
        level->setTileNoUpdate(x, y, z, Tile::rock->id);
    }
    if (this->liquidType == LiquidType::LAVA && (neighborTileId == Tile::water->id || neighborTileId == Tile::calmWater->id)) {
        level->setTileNoUpdate(x, y, z, Tile::rock->id);
    }

    level->addToTickNextTick(x, y, z, this->tileId);
}

short LiquidTile::getRenderPass() {
    return 1;
}