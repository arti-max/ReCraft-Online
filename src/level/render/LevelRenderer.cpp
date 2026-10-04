#include "level/render/LevelRenderer.hpp"
#include "GL/gl.h"
#include "render/Tessellator.hpp"
#include <iostream>
#include <cmath>

void debugGLState(const char* label) {
    printf("===== GL STATE: %s =====\n", label);
    
    printf("GL_TEXTURE_2D: %s\n", glIsEnabled(GL_TEXTURE_2D) ? "ON" : "OFF");
    printf("GL_LIGHTING: %s\n", glIsEnabled(GL_LIGHTING) ? "ON" : "OFF");
    printf("GL_COLOR_MATERIAL: %s\n", glIsEnabled(GL_COLOR_MATERIAL) ? "ON" : "OFF");
    printf("GL_BLEND: %s\n", glIsEnabled(GL_BLEND) ? "ON" : "OFF");
    printf("GL_ALPHA_TEST: %s\n", glIsEnabled(GL_ALPHA_TEST) ? "ON" : "OFF");
    
    printf("GL_VERTEX_ARRAY: %s\n", glIsEnabled(GL_VERTEX_ARRAY) ? "ON" : "OFF");
    printf("GL_TEXTURE_COORD_ARRAY: %s\n", glIsEnabled(GL_TEXTURE_COORD_ARRAY) ? "ON" : "OFF");
    printf("GL_COLOR_ARRAY: %s\n", glIsEnabled(GL_COLOR_ARRAY) ? "ON" : "OFF");
    printf("GL_NORMAL_ARRAY: %s\n", glIsEnabled(GL_NORMAL_ARRAY) ? "ON" : "OFF");
    
    GLint currentTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
    printf("Current texture: %d\n", currentTexture);
    
    GLfloat currentColor[4];
    glGetFloatv(GL_CURRENT_COLOR, currentColor);
    printf("Current color: (%.2f, %.2f, %.2f, %.2f)\n", 
           currentColor[0], currentColor[1], currentColor[2], currentColor[3]);
    
    GLint blendSrc, blendDst;
    glGetIntegerv(GL_BLEND_SRC, &blendSrc);
    glGetIntegerv(GL_BLEND_DST, &blendDst);
    printf("Blend func: src=%d, dst=%d\n", blendSrc, blendDst);
    
    printf("=============================\n\n");
}

LevelRenderer::LevelRenderer(Level* level, Textures* textures) 
    : level(level), textures(textures) {
    level->addListener(this);
    this->surroundLists = glGenLists(2);
    this->skyLists = glGenLists(2);
    allChanged();
}

LevelRenderer::~LevelRenderer() {
    for (Chunk* chunk : chunks) {
        delete chunk;
    }
    
    glDeleteLists(surroundLists, 2);
    glDeleteLists(skyLists, 2);

    if (this->level != nullptr) {
        this->level->removeListener(this);
    }
}

void LevelRenderer::allChanged() {
    lX = -900000.0f;
    lY = -900000.0f;
    lZ = -900000.0f;
    
    xChunks = (level->width + CHUNK_SIZE - 1) / CHUNK_SIZE;
    yChunks = (level->depth + CHUNK_SIZE - 1) / CHUNK_SIZE;
    zChunks = (level->height + CHUNK_SIZE - 1) / CHUNK_SIZE;
    
    for (Chunk* chunk : chunks) {
        delete chunk;
    }

    this->skyCompiled = false;
    chunks.clear();
    sortedChunks.clear();
    int totalChunks = xChunks * yChunks * zChunks;
    chunks.resize(totalChunks, nullptr);
    sortedChunks.resize(totalChunks, nullptr);
    
    for (int x = 0; x < xChunks; ++x) {
        for (int y = 0; y < yChunks; ++y) {
            for (int z = 0; z < zChunks; ++z) {
                int x0 = x * CHUNK_SIZE;
                int y0 = y * CHUNK_SIZE;
                int z0 = z * CHUNK_SIZE;
                int x1 = (x + 1) * CHUNK_SIZE;
                int y1 = (y + 1) * CHUNK_SIZE;
                int z1 = (z + 1) * CHUNK_SIZE;
                
                if (x1 > level->width) x1 = level->width;
                if (y1 > level->depth) y1 = level->depth;
                if (z1 > level->height) z1 = level->height;
                
                chunks[(x + y * xChunks) * zChunks + z] = new Chunk(level, x0, y0, z0, x1, y1, z1);
                sortedChunks[(x + y * xChunks) * zChunks + z] = this->chunks[(x + y * xChunks) * zChunks + z];
            }
        }
    }
    
    glNewList(this->surroundLists + 0, GL_COMPILE);
    compileSurroundingGround();
    glEndList();
    
    glNewList(this->surroundLists + 1, GL_COMPILE);
    compileSurroundingWater();
    glEndList();
    
    for (Chunk* chunk : chunks) {
        chunk->reset();
    }

    for (Chunk* chunk : chunks) {
        chunk->rebuild();
    }
    
    std::cout << "LevelRenderer initialized: " << chunks.size() << " chunks" << std::endl;
}

std::vector<Chunk*> LevelRenderer::getAllDirtyChunks() {
    std::vector<Chunk*> dirty;
    
    for (Chunk* chunk : chunks) {
        if (chunk->isDirty()) {
            dirty.push_back(chunk);
        }
    }
    
    return dirty;
}

int LevelRenderer::render(Player* player, int layer) {
    float xd = player->x - lX;
    float yd = player->y - lY;
    float zd = player->z - lZ;
    if (xd * xd + yd * yd + zd * zd > 64.0f) {
        lX = player->x;
        lY = player->y;
        lZ = player->z;
        std::sort(sortedChunks.begin(), sortedChunks.end(), DistanceSorter(player, layer));
    }

    displayListCache.clear();

    float dd = (float)(256.0f / (1 << drawDistance));
    float maxDistSqr = dd*dd;
    
    for (Chunk* chunk : sortedChunks) {
        if (chunk->visible) {
            if (drawDistance == 0 || chunk->distanceToSqr(player) < maxDistSqr) {
                chunk->appendLists(displayListCache, layer);
            }
        }
    }

    this->renderCollectedChunks();
    
    return (int)(displayListCache.size());
}

void LevelRenderer::renderCollectedChunks() {
    if (!displayListCache.empty()) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textures->loadTexture("terrain", GL_NEAREST));
        glCallLists((GLsizei)(displayListCache.size()), GL_INT, displayListCache.data());
    }
    glDisable(GL_TEXTURE_2D);
}

void LevelRenderer::renderSurroundingGround() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures->loadTexture("rock2", GL_NEAREST));
    glCallList(this->surroundLists + 0);
}

void LevelRenderer::compileSurroundingGround() {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_FOG);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    Tessellator& t = Tessellator::getInstance();
    float y = level->getGroundLevel();
    int s = 128;
    
    if (s > level->width) s = level->width;
    if (s > level->height) s = level->height;
    
    int d = 5;
    t.begin();
    
    for (int xx = -s * d; xx < level->width + s * d; xx += s) {
        for (int zz = -s * d; zz < level->height + s * d; zz += s) {
            float yy = y;
            if (xx >= 0 && zz >= 0 && xx < level->width && zz < level->height) {
                yy = 0.0f;
            }
            
            t.vertexUV(static_cast<float>(xx), yy, static_cast<float>(zz + s), 0.0f, static_cast<float>(s));
            t.vertexUV(static_cast<float>(xx + s), yy, static_cast<float>(zz + s), static_cast<float>(s), static_cast<float>(s));
            t.vertexUV(static_cast<float>(xx + s), yy, static_cast<float>(zz), static_cast<float>(s), 0.0f);
            t.vertexUV(static_cast<float>(xx), yy, static_cast<float>(zz), 0.0f, 0.0f);
        }
    }
    
    t.end();
    
    glColor3f(0.8f, 0.8f, 0.8f);
    t.begin();
    
    for (int xx = 0; xx < level->width; xx += s) {
        t.vertexUV(static_cast<float>(xx), 0.0f, 0.0f, 0.0f, 0.0f);
        t.vertexUV(static_cast<float>(xx + s), 0.0f, 0.0f, static_cast<float>(s), 0.0f);
        t.vertexUV(static_cast<float>(xx + s), y, 0.0f, static_cast<float>(s), y);
        t.vertexUV(static_cast<float>(xx), y, 0.0f, 0.0f, y);
        
        t.vertexUV(static_cast<float>(xx), y, static_cast<float>(level->height), 0.0f, y);
        t.vertexUV(static_cast<float>(xx + s), y, static_cast<float>(level->height), static_cast<float>(s), y);
        t.vertexUV(static_cast<float>(xx + s), 0.0f, static_cast<float>(level->height), static_cast<float>(s), 0.0f);
        t.vertexUV(static_cast<float>(xx), 0.0f, static_cast<float>(level->height), 0.0f, 0.0f);
    }
    
    for (int zz = 0; zz < level->height; zz += s) {
        t.vertexUV(0.0f, 0.0f, static_cast<float>(zz), 0.0f, 0.0f);
        t.vertexUV(0.0f, y, static_cast<float>(zz), 0.0f, y);
        t.vertexUV(0.0f, y, static_cast<float>(zz + s), static_cast<float>(s), y);
        t.vertexUV(0.0f, 0.0f, static_cast<float>(zz + s), static_cast<float>(s), 0.0f);
        
        t.vertexUV(static_cast<float>(level->width), 0.0f, static_cast<float>(zz + s), static_cast<float>(s), 0.0f);
        t.vertexUV(static_cast<float>(level->width), y, static_cast<float>(zz + s), static_cast<float>(s), y);
        t.vertexUV(static_cast<float>(level->width), y, static_cast<float>(zz), 0.0f, y);
        t.vertexUV(static_cast<float>(level->width), 0.0f, static_cast<float>(zz), 0.0f, 0.0f);
    }
    
    t.end();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
}

void LevelRenderer::renderSurroundingWater() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures->loadTexture("water", GL_NEAREST));
    glCallList(this->surroundLists + 1);
}

void LevelRenderer::compileSurroundingWater() {
    glEnable(GL_FOG);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    
    float y = level->getWaterLevel();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    Tessellator& t = Tessellator::getInstance();
    int s = 128;
    if (s > level->width) s = level->width;
    if (s > level->height) s = level->height;
    
    int d = 5;
    t.begin();
    
    for (int xx = -s * d; xx < level->width + s * d; xx += s) {
        for (int zz = -s * d; zz < level->height + s * d; zz += s) {
            float yy = y - 0.1f;
            if (xx < 0 || zz < 0 || xx >= level->width || zz >= level->height) {
                t.vertexUV(static_cast<float>(xx), yy, static_cast<float>(zz + s), 0.0f, static_cast<float>(s));
                t.vertexUV(static_cast<float>(xx + s), yy, static_cast<float>(zz + s), static_cast<float>(s), static_cast<float>(s));
                t.vertexUV(static_cast<float>(xx + s), yy, static_cast<float>(zz), static_cast<float>(s), 0.0f);
                t.vertexUV(static_cast<float>(xx), yy, static_cast<float>(zz), 0.0f, 0.0f);
                
                t.vertexUV(static_cast<float>(xx), yy, static_cast<float>(zz), 0.0f, 0.0f);
                t.vertexUV(static_cast<float>(xx + s), yy, static_cast<float>(zz), static_cast<float>(s), 0.0f);
                t.vertexUV(static_cast<float>(xx + s), yy, static_cast<float>(zz + s), static_cast<float>(s), static_cast<float>(s));
                t.vertexUV(static_cast<float>(xx), yy, static_cast<float>(zz + s), 0.0f, static_cast<float>(s));
            }
        }
    }
    
    t.end();
    
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
}

void LevelRenderer::updateDirtyChunks(Player* player) {
    std::vector<Chunk*> dirty = getAllDirtyChunks();
    if (!dirty.empty()) {
        std::sort(dirty.begin(), dirty.end(), DirtyChunkSorter(player));
        
        int rebuiltCount = 0;
        for (Chunk* chunk : dirty) {
            if (rebuiltCount >= MAX_REBUILDS_PER_FRAME) break;
            chunk->rebuild();
            rebuiltCount++;
        }
    }
}

void LevelRenderer::setDirty(int x0, int y0, int z0, int x1, int y1, int z1) {
    x0 /= CHUNK_SIZE;
    x1 /= CHUNK_SIZE;
    y0 /= CHUNK_SIZE;
    y1 /= CHUNK_SIZE;
    z0 /= CHUNK_SIZE;
    z1 /= CHUNK_SIZE;
    
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (z0 < 0) z0 = 0;
    if (x1 >= xChunks) x1 = xChunks - 1;
    if (y1 >= yChunks) y1 = yChunks - 1;
    if (z1 >= zChunks) z1 = zChunks - 1;
    
    for (int x = x0; x <= x1; ++x) {
        for (int y = y0; y <= y1; ++y) {
            for (int z = z0; z <= z1; ++z) {
                int index = (x + y * xChunks) * zChunks + z;
                if (index < chunks.size()) {
                    chunks[index]->setAllDirty();
                }
            }
        }
    }
}

void LevelRenderer::renderHit(HitResult* h, Player* player, int mode, int tileType) {
    Tessellator& t = Tessellator::getInstance();
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1.0f, 1.0f, 1.0f, ((float) std::sin(emscripten_get_now() / 100.0f) * 0.2f + 0.4f) * 0.5f);
    if (mode == 0) {
        t.begin();

        for (int i = 0; i < 6; ++i) {
            Tile::tiles[Tile::rock->id]->renderFaceNoTexture(player, t, h->x, h->y, h->z, i);
        }

        t.end();
    } else {
        if (tileType != -1) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            float br = (float)std::sin((double)emscripten_get_now() / 100.0f) * 0.2f + 0.8f;
            glColor4f(br, br, br, (float)std::sin((double)emscripten_get_now() / 200.0f) * 0.2f + 0.5f);
            glEnable(GL_TEXTURE_2D);
            int id = this->textures->loadTexture("terrain", GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, id);
            int x = h->x;
            int y = h->y;
            int z = h->z;
            if (h->f == 0) y--;
            if (h->f == 1) y++;
            if (h->f == 2) z--;
            if (h->f == 3) z++;
            if (h->f == 4) x--;
            if (h->f == 5) x++;

            t.begin();
            t._noColor();
            Tile::tiles[tileType]->render(t, this->level, x, y, z);
            t.end();
            glDisable(GL_TEXTURE_2D);
        }
    }

    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
}

void LevelRenderer::renderHitOutline(HitResult* h, Player* player, int mode, int tileType) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    float x = (float)h->x;
    float y = (float)h->y;
    float z = (float)h->z;
    if (mode == 1) {
        if (h->f == 0) y--;
        if (h->f == 1) y++;
        if (h->f == 2) z--;
        if (h->f == 3) z++;
        if (h->f == 4) x--;
        if (h->f == 5) x++;
    }   

    int tileId = this->level->getTile(x, y, z);

    if (h->type == 0 && tileId > 0) {
        Tile* tile = Tile::tiles[tileId];
        if (tile == nullptr) return;

        float minX = tile->minX;
        float minY = tile->minY;
        float minZ = tile->minZ;

        float maxX = tile->maxX;
        float maxY = tile->maxY;
        float maxZ = tile->maxZ;

        glBegin(GL_LINE_STRIP);
        glVertex3f(x + minX, y + minY, z + minZ);
        glVertex3f(x + maxX, y + minY, z + minZ);
        glVertex3f(x + maxX, y + minY, z + maxZ);
        glVertex3f(x+ minX, y + minY, z + maxZ);
        glVertex3f(x+ minX, y + minY, z + minZ);
        glEnd();
        glBegin(3);
        glVertex3f(x+ minX, y + maxY, z + minZ);
        glVertex3f(x + maxX, y + maxY, z + minZ);
        glVertex3f(x + maxX, y + maxY, z + maxZ);
        glVertex3f(x+ minX, y + maxY, z + maxZ);
        glVertex3f(x+ minX, y + maxY, z + minZ);
        glEnd();
        glBegin(1);
        glVertex3f(x+ minX, y + minY, z + minZ);
        glVertex3f(x+ minX, y + maxY, z + minZ);
        glVertex3f(x + maxX, y + minY, z + minZ);
        glVertex3f(x + maxX, y + maxY, z + minZ);
        glVertex3f(x + maxX, y + minY, z + maxZ);
        glVertex3f(x + maxX, y + maxY, z + maxZ);
        glVertex3f(x + minX, y + minY, z + maxZ);
        glVertex3f(x + minX, y + maxY, z + maxZ);
        glEnd();
        glDisable(GL_BLEND);
    }
}

void LevelRenderer::tileChanged(int x, int y, int z) {
    setDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}

void LevelRenderer::lightColumnChanged(int x, int z, int y0, int y1) {
    setDirty(x - 1, y0 - 1, z - 1, x + 1, y1 + 1, z + 1);
}

void LevelRenderer::toggleDrawDistance() {
    drawDistance = (drawDistance + 1) % 4;
    std::cout << "Draw distance: " << drawDistance << std::endl;
}

void LevelRenderer::cull(Frustum& frustum) {
    for (Chunk* chunk : chunks) {
        if (!frustum.sphereInFrustum(chunk->x, chunk->y, chunk->z, chunk->boundingSphereRadius)) {
            chunk->visible = false;
        } else {
            chunk->visible = frustum.isVisible(chunk->boundingBox);
        }
    }
}

void LevelRenderer::renderClouds(float partialTicks) {
    glEnable(GL_FOG);
    glCullFace(GL_BACK);
    this->renderSky();
    glCullFace(GL_FRONT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures->loadTexture("clouds", GL_NEAREST));
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    Tessellator& t = Tessellator::getInstance();
    
    float var3 = 0.0f;
    float var4 = 4.8828125E-4f;
    var3 = static_cast<float>(level->depth + 2);
    float var1 = (static_cast<float>(cloudTicks) + partialTicks) * var4 * 0.03f;
    
    t.begin();
    t.color(1.0f, 1.0f, 1.0f);
    
    for (int var8 = -2048; var8 < level->width + 2048; var8 += 512) {
        for (int var6 = -2048; var6 < level->height + 2048; var6 += 512) {
            t.vertexUV((float)var8, var3, (float)(var6 + 512), (float)var8 * var4 + var1, (float)(var6 + 512) * var4);
            t.vertexUV((float)(var8 + 512), var3, (float)(var6 + 512), (float)(var8 + 512) * var4 + var1, (float)(var6 + 512) * var4);
            t.vertexUV((float)(var8 + 512), var3, (float)var6, (float)(var8 + 512) * var4 + var1, (float)var6 * var4);
            t.vertexUV((float)var8, var3, (float)var6, (float)var8 * var4 + var1, (float)var6 * var4);
            
            t.vertexUV((float)var8, var3, (float)var6, (float)var8 * var4 + var1, (float)var6 * var4);
            t.vertexUV((float)(var8 + 512), var3, (float)var6, (float)(var8 + 512) * var4 + var1, (float)var6 * var4);
            t.vertexUV((float)(var8 + 512), var3, (float)(var6 + 512), (float)(var8 + 512) * var4 + var1, (float)(var6 + 512) * var4);
            t.vertexUV((float)var8, var3, (float)(var6 + 512), (float)var8 * var4 + var1, (float)(var6 + 512) * var4);
        }
    }

    t.end();
    glDisable(GL_TEXTURE_2D);
}

void LevelRenderer::renderSky() {
    if (!this->skyCompiled) {
        glNewList(this->skyLists, GL_COMPILE);
        glDisable(GL_TEXTURE_2D);

        Tessellator& t = Tessellator::getInstance();

        glColor3f(0.5f, 0.8f, 1.0f);

        float skyY = static_cast<float>(this->level->depth + 10);
        
        // glBegin(GL_QUADS);
        t.begin();
        t.color(0.5f, 0.8f, 1.0f);

        for (int x = -2048; x < this->level->width + 2048; x += 512) {
            for (int z = -2048; z < this->level->height + 2048; z += 512) {
                t.vertex(static_cast<float>(x), skyY, static_cast<float>(z));
                t.vertex(static_cast<float>(x + 512), skyY, static_cast<float>(z));
                t.vertex(static_cast<float>(x + 512), skyY, static_cast<float>(z + 512));
                t.vertex(static_cast<float>(x), skyY, static_cast<float>(z + 512));
            }
        }
        
        // glEnd();
        t.end();
        glEndList();
        this->skyCompiled = true;
    } else {
        glCallList(this->skyLists);
    }
}