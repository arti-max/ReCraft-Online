#pragma once
#include <array>
#include "render/Tessellator.hpp"
#include "level/Level.hpp"
#include "util/Random.hpp"
#include "phys/AABB.hpp"
#include "player/Player.hpp"
#include "particle/ParticleEngine.hpp"
#include "sound/SoundType.hpp"
#include "level/liquid/LiquidType.hpp"
#include "HitResult.hpp"

class Tile {

private:

protected:
    virtual bool shouldRenderFace(Level* level, int x, int y, int z, int face);
public:
    Tile(int id);
    Tile (int id, int textureId);
    ~Tile();
    static std::array<Tile*, 256> tiles;
    static const Tile* empty;
    static const Tile* rock;
    static const Tile* grass;
    static const Tile* dirt;
    static const Tile* cobblestone;
    static const Tile* wood;
    static const Tile* bush;
    static const Tile* water;
    static const Tile* unbreakable;
    static const Tile* calmWater;
    static const Tile* lava;
    static const Tile* calmLava;
    static const Tile* gravel;
    static const Tile* sand;
    static const Tile* log;
    static const Tile* leaves;
    static const Tile* goldOre;
    static const Tile* ironOre;
    static const Tile* coalOre;
    static const Tile* sponge;
    static const Tile* glass;
    static const Tile* wool1;
    static const Tile* wool2;
    static const Tile* wool3;
    static const Tile* wool4;
    static const Tile* wool5;
    static const Tile* wool6;
    static const Tile* wool7;
    static const Tile* wool8;
    static const Tile* wool9;
    static const Tile* wool10;
    static const Tile* wool11;
    static const Tile* wool12;
    static const Tile* wool13;
    static const Tile* wool14;
    static const Tile* wool15;
    static const Tile* wool16;
    static const Tile* redFlower;
    static const Tile* yellowFlower;
    static const Tile* redMushroom;
    static const Tile* brownMushroom;
    static const Tile* goldBlock;
    static const Tile* ironBlock;
    static const Tile* slab;
    static const Tile* doubleSlab;
    static const Tile* tnt;
    static const Tile* bricks;
    static const Tile* mossStone;
    static const Tile* bookshelf;
    static const Tile* cobweb;
    static const Tile* obsidian;

    int textureId = 0;
    int id = 0;
    int dropid = -1;
    int dropcnt = 1;
    int hardness = 0;
    float particleGravity = 0.0f;
    float minX = 0.0f, minY = 0.0f, minZ = 0.0f, maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
    const SoundType* st;

    virtual Tile* setData(const SoundType& st, float particleGravity, float hardness);
    virtual Tile* setDrop(int id);
    virtual int getTexture(int face);
    virtual bool render(Tessellator& t, Level* level, int x, int y, int z);
    virtual void renderFace(Tessellator& t, int x, int y, int z, int face);
    virtual void renderFace(Tessellator& t, int x, int y, int z, int face, int textureId);
    virtual void renderBackFace(Tessellator& t, int x, int y, int z, int face);
    virtual void renderFaceNoTexture(Player* player, Tessellator& t, int x, int y, int z, int face);
    virtual void onDestroy(Level* level, int x, int y, int z, ParticleEngine* engine, bool drop);
    virtual void onDestroy(Level* level, int x, int y, int z);
    virtual void tick(Level* level, int x, int y, int z, Random* random);
    virtual bool mayPick();
    virtual bool blocksLight();
    virtual bool isSolid();
    virtual AABB* getTileAABB(int x, int y, int z) const;
    virtual AABB* getAABB(int x, int y, int z) const ;
    virtual void neighborChanged(Level* level, int x, int y, int z, int type);
    virtual LiquidType getLiquidType();
    virtual bool isCalmLiquid();
    virtual void onBlockAdded(Level* level, int x, int y, int z);
    virtual float getBrightness(Level* level, int x, int y, int z);
    virtual int getDrop();
    virtual int getDropCount();
    virtual int getHardness();
    virtual void spawnDrop(Level* level, int x, int y, int z, float chance);
    virtual void spawnDestroyParticles(Level* level, int x, int y, int z, ParticleEngine* engine);
    virtual void spawnHitParticles(Level* level, int x, int y, int z, int face, ParticleEngine* engine);
    virtual HitResult* clip(int x, int y, int z, Vec3D& start, Vec3D& end);
    virtual void renderPreview(Tessellator& t);
    virtual void setShape(float x0, float y0, float z0, float x1, float y1, float z1);
    virtual short getRenderPass();
};
