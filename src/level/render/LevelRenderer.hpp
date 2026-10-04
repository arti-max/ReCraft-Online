#pragma once
#include <vector>
#include <algorithm>
#include "level/render/LevelListener.hpp"
#include "level/Level.hpp"
#include "level/Chunk.hpp"
#include "level/tile/Tile.hpp"
#include "phys/AABB.hpp"
#include "render/Frustum.hpp"
#include "render/Textures.hpp"
#include "HitResult.hpp"
#include "player/Player.hpp"
#include "level/sort/DistanceSorter.hpp"
#include "level/sort/DirtyChunkSorter.hpp"
#include "character/Vec3.hpp"
#include "gui/Font.hpp"
#include <GL/gl.h>

struct NameTagInfo {
    std::string text = "";
    Vec3 position;
    float scale = 0.0f;
};

class LevelRenderer : public LevelListener {
private:
    Level* level;
    Textures* textures;
    std::vector<Chunk*> chunks;
    std::vector<Chunk*> sortedChunks;
    int xChunks = 0, yChunks = 0, zChunks = 0;
    GLuint surroundLists;
    GLuint skyLists;
    float lX = 0.0f;
    float lY = 0.0f;
    float lZ = 0.0f;
    bool skyCompiled = false;
    std::vector<NameTagInfo> nameTagsToRender;
    std::vector<GLint> displayListCache;

public:
    static const int MAX_REBUILDS_PER_FRAME = 4;
    static const int CHUNK_SIZE = 16;
    
    int cloudTicks = 0;
    int drawDistance = 0;
    float cracks = 0.4f;

    LevelRenderer(Level* level, Textures* textures);
    ~LevelRenderer();
    
    void allChanged() override;
    std::vector<Chunk*> getAllDirtyChunks();
    int render(Player* player, int layer);
    void renderCollectedChunks();
    void renderSurroundingGround();
    void compileSurroundingGround();
    void renderSurroundingWater();
    void compileSurroundingWater();
    void updateDirtyChunks(Player* player);
    void setDirty(int x0, int y0, int z0, int x1, int y1, int z1);
    void tileChanged(int x, int y, int z) override;
    void lightColumnChanged(int x, int z, int y0, int y1) override;
    void toggleDrawDistance();
    void cull(Frustum& frustum);
    void renderHit(HitResult* h, Player* player, int mode, int tileType);
    void renderHitOutline(HitResult* h, Player* player, int mode, int tileType);
    void renderClouds(float partialTicks);
    void renderSky();
    void addNameTagToRender(const std::string& text, const Vec3& pos, float scale);
    void renderNameTags(Font* font, Player* localPlayer);
};
