#include "level/Chunk.hpp"
#include "GL/gl.h"

int Chunk::rebuiltThisFrame = 0;
int Chunk::updates = 0;

Tessellator& Chunk::t = Tessellator::getInstance();

Chunk::Chunk(Level* level, int x0, int y0, int z0, int x1, int y1, int z1) : 
    level(level),
    boundingBox((float)x0, (float)y0, (float)z0, (float)x1, (float)y1, (float)z1) {
    this->x0 = x0;
    this->y0 = y0;
    this->z0 = z0;
    this->x1 = x1;
    this->y1 = y1;
    this->z1 = z1;
    
    this->x = (x0 + x1) / 2.0f;
    this->y = (y0 + y1) / 2.0f;
    this->z = (z0 + z1) / 2.0f;

    float dx = x1 - this->x;
    float dy = y1 - this->y;
    float dz = z1 - this->z;
    this->boundingSphereRadius = std::sqrt(dx*dx + dy*dy + dz*dz);

    this->lists = glGenLists(2);
    this->setAllDirty();
}

Chunk::~Chunk() {
    if (this->lists > 0) {
        glDeleteLists(this->lists, 2);
    }
}

void Chunk::rebuild() {
    this->updates++;
    Chunk::rebuiltThisFrame++;

    glEnable(GL_TEXTURE_2D);
    Tile* currentTile = nullptr;

    for (short layer = 0; layer < 2 ; layer++) {
        this->dirty[layer] = true;
    }

    for (short layer = 0; layer < 2; layer++) {
        bool hasAnyGeometry = false;
        bool hasNextRenderPass = false;

        glNewList(this->lists + layer, GL_COMPILE);
        this->t.begin();

        for (int x = this->x0; x < this->x1; ++x) {
            for (int y = this->y0; y < this->y1; ++y) {
                for (int z = this->z0; z < this->z1; ++z) {
                    int id = this->level->getTile(x, y, z);
                    if (id > 0) {
                        currentTile = Tile::tiles[id];
                        if (currentTile != nullptr) {
                            if (currentTile->getRenderPass() != layer) {
                                hasNextRenderPass = true;
                            } else {
                                hasAnyGeometry |= currentTile->render(this->t, this->level, x, y, z);
                            }
                        }
                    }
                }
            }
        }

        this->t.end();
        glEndList();
        this->dirty[layer] = false;
        if (!hasNextRenderPass) {
            for (short next = layer + 1; next < 2; next++) {
                glNewList(this->lists + next, GL_COMPILE);
                glEndList();
                this->dirty[next] = false;
            }
            break;
        }

    }
}

void Chunk::render(int layer) {
    if (!this->dirty[layer]) {
        glCallList(this->lists + layer);
    }
}

bool Chunk::isDirty() {
    return this->dirty[0] || this->dirty[1];
}

void Chunk::setAllDirty() {
    for (short layer = 0; layer < 2; layer++) {
        this->dirty[layer] = true;
    }
}

void Chunk::reset() {
    for (int i = 0; i < 2; ++i) {
        this->dirty[i] = true;
        glNewList(this->lists + i, GL_COMPILE);
        glEndList();
    }
}

float Chunk::distanceToSqr(Player* player) {
    float xd = player->x - this->x;
    float yd = player->y - this->y;
    float zd = player->z - this->z;
    return xd*xd + yd*yd + zd*zd;
}

void Chunk::appendLists(std::vector<GLint>& listsArr, int renderPass) {
    if (!this->visible) {
        return;
    } 
    if (!this->dirty[renderPass]) {
        listsArr.push_back(this->lists + renderPass);
    }
}