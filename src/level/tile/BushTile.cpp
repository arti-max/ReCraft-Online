#include "level/tile/BushTile.hpp"

Bush::Bush(int id, int textureId) : Tile(id) {
    this->textureId = textureId;
    float size = 0.4f;
    this->setShape(0.5f - size, 0.0f, 0.5f - size, size + 0.5f, size * 2.0f, size + 0.5f);
}

void Bush::tick(Level* level, int x, int y, int z, Random* random) {
    int tileIdBelow = level->getTile(x, y-1, z);
    if (!level->isLit(x, y, z) || (tileIdBelow != Tile::grass->id && tileIdBelow != Tile::dirt->id)) {
        level->setTile(x, y, z, 0);
    } else {
        if (random->nextInt(5) == 0 || level->growTrees) {
            level->setTileNoUpdate(x, y, z, 0);
            if (!level->maybeGrowTree(x, y, z)) {
                level->setTileNoUpdate(x, y, z, this->id);
            }
        }
    }
}

bool Bush::render(Tessellator& t, Level* level, int x, int y, int z) {
    float color = level->getBrightness(x, y, z);
    t.color(color, color, color);
    this->render(t, (float)x, (float)y, (float)z);
    return true;
}

bool Bush::render(Tessellator& t, float x, float y, float z) {
    int textureId = this->textureId;

    const float atlasSize = 16.0f;
    const float tilePixels = 16.0f;
    const float atlasPixels = 256.0f;

    const float col = static_cast<float>(textureId % static_cast<int>(atlasSize));
    const float row = static_cast<float>(textureId / static_cast<int>(atlasSize));

    const float u0 = col * tilePixels / atlasPixels;
    const float v0 = row * tilePixels / atlasPixels;
    const float u1 = u0 + tilePixels / atlasPixels;
    const float v1 = v0 + tilePixels / atlasPixels;

    t.color(1.0f, 1.0f, 1.0f);

    for (int i = 0; i < 2; ++i) {
        float xOffset = (float)((double)std::sin((float)i * 3.1415927F / 2.0F + 0.7853982F) * 0.5f);
        float zOffset = (float)((double)std::cos((float)i * 3.1415927F / 2.0F + 0.7853982F) * 0.5f);

        float x0 = x + 0.5f - xOffset;
        float x1 = x + 0.5f + xOffset;
        float y0 = y + 0.0f;
        float y1 = y + 1.0f;
        float z0 = z + 0.5f - zOffset;
        float z1 = z + 0.5f + zOffset;

        t.vertexUV(x0, y1, z0, u1, v0);
        t.vertexUV(x1, y1, z1, u0, v0);
        t.vertexUV(x1, y0, z1, u0, v1);
        t.vertexUV(x0, y0, z0, u1, v1);

        t.vertexUV(x1, y1, z1, u1, v0);
        t.vertexUV(x0, y1, z0, u0, v0);
        t.vertexUV(x0, y0, z0, u0, v1);
        t.vertexUV(x1, y0, z1, u1, v1);
    }

    return true;
}

AABB* Bush::getAABB(int x, int y, int z) const {
    return nullptr;
}

bool Bush::blocksLight() {
    return false;
}

bool Bush::isSolid() {
    return false;
}

void Bush::renderPreview(Tessellator& t) {
    t.normal(0.0f, 1.0f, 0.0f);
    t.begin();
    this->render(t, 0.0f, 0.4f, -0.3f);
    t.end();
}