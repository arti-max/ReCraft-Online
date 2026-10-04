#include "LevelGen.hpp"
#include "level/levelgen/synth/OctaveNoise.hpp"
#include "level/levelgen/synth/CombinedNoise.hpp"
#include "level/tile/Tile.hpp"
#include "mob/Zombie.hpp"
#include "mob/Skeleton.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

#define SAFE_DIV(val, max_val) ((max_val) > 1 ? ((val) * 100 / ((max_val) - 1)) : 100)

LevelGen::LevelGen(LevelLoaderListener* listener) : listener(listener), random() {}
LevelGen::~LevelGen() = default;

void LevelGen::generateLevel(Level* level, const char* username, int w, int h, int d) {
    listener->beginLevelLoading("Generating level");
    this->width = w;
    this->depth = d;
    this->height = h;

    blocks.assign(width * height * depth, 0);
    heightmap.assign(width * height, 0);

    listener->levelLoadUpdate("Raising..");
    raise(heightmap);

    listener->levelLoadUpdate("Eroding..");
    erode(heightmap);

    listener->levelLoadUpdate("Soiling..");
    soil(heightmap);

    listener->levelLoadUpdate("Carving..");
    carve();

    listener->levelLoadUpdate("Watering..");
    addWaterAndLava();

    listener->levelLoadUpdate("Growing..");
    growSurface(heightmap);

    listener->levelLoadUpdate("Planting..");
    addSurfaceFlowers(heightmap);
    addUndergroundMushrooms(heightmap);


    level->setData(width, depth, height, blocks);
    level->creationTime = time(nullptr);
    level->creator = username;
    level->name = "A Nice World";

    listener->levelLoadUpdate("Growing trees..");
    addTrees(level, heightmap);

    // listener->levelLoadUpdate("Spawning..");
    // spawnMobs(level);

    listener->levelLoadUpdate("Done");
    listener->levelLoadProgress(100);
}

void LevelGen::raise(std::vector<int>& map) {
    CombinedNoise noise1(std::make_unique<OctaveNoise>(random, 8),
                         std::make_unique<OctaveNoise>(random, 8));
    CombinedNoise noise2(std::make_unique<OctaveNoise>(random, 8),
                         std::make_unique<OctaveNoise>(random, 8));
    OctaveNoise cliffNoise(random, 8);
    const float scale = 1.3f;

    for (int x = 0; x < width; ++x) {
        listener->levelLoadProgress(x * 100 / (width - 1));
        for (int z = 0; z < height; ++z) {
            double h1 = noise1.compute(x * scale, z * scale) / 8.0 - 8.0;
            double h2 = noise2.compute(x * scale, z * scale) / 6.0 + 6.0;
            if (cliffNoise.compute(x, z) / 8.0 > 0.0) h2 = h1;

            double val = std::max(h1, h2) / 2.0;
            if (val < 0.0) val *= 0.8;
            map[x + z * width] = static_cast<int>(val);
        }
    }
}

void LevelGen::erode(std::vector<int>& map) {
    CombinedNoise erode1(std::make_unique<OctaveNoise>(random, 8),
                         std::make_unique<OctaveNoise>(random, 8));
    CombinedNoise erode2(std::make_unique<OctaveNoise>(random, 8),
                         std::make_unique<OctaveNoise>(random, 8));

    for (int x = 0; x < width; ++x) {
        listener->levelLoadProgress(x * 100 / (width - 1));
        for (int z = 0; z < height; ++z) {
            double val = erode1.compute(x * 2.0, z * 2.0) / 8.0;
            int erosion = erode2.compute(x * 2.0, z * 2.0) > 0.0 ? 1 : 0;
            if (val > 2.0) {
                int h = map[x + z * width];
                h = ((h - erosion) / 2) * 2 + erosion;
                map[x + z * width] = h;
            }
        }
    }
}

void LevelGen::soil(std::vector<int>& map) {
    OctaveNoise soilNoise(random, 8);
    const int waterLevel = depth / 2;

    for (int x = 0; x < width; ++x) {
        listener->levelLoadProgress(x * 100 / (width - 1));
        for (int z = 0; z < height; ++z) {
            int soilDepth = static_cast<int>(soilNoise.compute(x, z) / 24.0) - 4;
            int dirtTopY = map[x + z * width] + waterLevel;
            int rockTopY = dirtTopY + soilDepth;
            map[x + z * width] = std::max(dirtTopY, rockTopY);

            for (int y = 0; y < depth; ++y) {
                int idx = (y * height + z) * width + x;
                uint8_t id = 0;
                if (y <= dirtTopY) id = Tile::dirt->id;
                if (y <= rockTopY) id = Tile::rock->id;
                blocks[idx] = id;
            }
        }
    }
}

void LevelGen::carve() {
    const int numCaves = width * height * depth / 256 / 64;
    for (int i = 0; i < numCaves; ++i) {
        listener->levelLoadProgress(SAFE_DIV(i, numCaves));

        float x = random.nextFloat() * width;
        float y = random.nextFloat() * depth;
        float z = random.nextFloat() * height;
        int length = static_cast<int>((random.nextFloat() + random.nextFloat()) * 200.0f);

        float yaw   = random.nextFloat() * 2.0f * M_PI;
        float pitch = 0.0f;
        float yawMod   = 0.0f;
        float pitchMod = 0.0f;

        for (int l = 0; l < length; ++l) {
            x += std::sin(yaw) * std::cos(pitch);
            z += std::cos(yaw) * std::cos(pitch);
            y += std::sin(pitch);

            yaw += yawMod * 0.2f;
            yawMod = (yawMod * 0.9f) + (random.nextFloat() - random.nextFloat());
            pitch = (pitch + pitchMod * 0.5f) * 0.5f;
            pitchMod = (pitchMod * 0.75f) + (random.nextFloat() - random.nextFloat());

            if (random.nextFloat() < 0.25f) continue;

            float depthRatio = 1.0f - (y / depth);
            float sizeFactor = 1.2f + (depthRatio * 3.5f + 1.0f) * (random.nextFloat() * random.nextFloat());
            float radius = std::sin(l * M_PI / length) * sizeFactor;

            int minX = std::max(1, (int)(x - radius));
            int maxX = std::min(width - 2, (int)(x + radius));
            int minY = std::max(1, (int)(y - radius));
            int maxY = std::min(depth - 2, (int)(y + radius));
            int minZ = std::max(1, (int)(z - radius));
            int maxZ = std::min(height - 2, (int)(z + radius));

            for (int ix = minX; ix <= maxX; ++ix) {
                for (int iy = minY; iy <= maxY; ++iy) {
                    for (int iz = minZ; iz <= maxZ; ++iz) {
                        float dx = ix - x;
                        float dy = iy - y;
                        float dz = iz - z;
                        if (dx*dx + dy*dy*2.0f + dz*dz < radius*radius) {
                            int idx = (iy * height + iz) * width + ix;
                            if (blocks[idx] == Tile::rock->id)
                                blocks[idx] = 0;
                        }
                    }
                }
            }
        }
    }

    listener->levelLoadUpdate("Adding minerals..");
    addOres(Tile::coalOre->id, 90, 1);
    addOres(Tile::ironOre->id, 75, 2);
    addOres(Tile::goldOre->id, 50, 3);
    listener->levelLoadProgress(100);
}

void LevelGen::addOres(int tileId, int count, int abundance) {
    int veinsCount = width * height * depth / 256 / 64 * count / 100;
    for (int i = 0; i < veinsCount; ++i) {
        listener->levelLoadProgress(SAFE_DIV(i, veinsCount) / 4 + abundance * 100 / 4);
        float x = random.nextFloat() * width;
        float y = random.nextFloat() * depth;
        float z = random.nextFloat() * height;
        int length = static_cast<int>((random.nextFloat() + random.nextFloat()) * 75.0f * (count / 100.0f));

        float yaw   = random.nextFloat() * 2.0f * M_PI;
        float pitch = random.nextFloat() * 2.0f * M_PI;
        float yawMod = 0.0f, pitchMod = 0.0f;

        for (int l = 0; l < length; ++l) {
            x += std::sin(yaw) * std::cos(pitch);
            z += std::cos(yaw) * std::cos(pitch);
            y += std::sin(pitch);

            yaw += yawMod * 0.2f;
            yawMod = (yawMod * 0.9f) + (random.nextFloat() - random.nextFloat());
            pitch = (pitch + pitchMod * 0.5f) * 0.5f;
            pitchMod = (pitchMod * 0.9f) + (random.nextFloat() - random.nextFloat());

            float radius = std::sin(l * M_PI / length) * (count / 100.0f) + 1.0f;
            int minX = std::max(1, (int)(x - radius));
            int maxX = std::min(width - 2, (int)(x + radius));
            int minY = std::max(1, (int)(y - radius));
            int maxY = std::min(depth - 2, (int)(y + radius));
            int minZ = std::max(1, (int)(z - radius));
            int maxZ = std::min(height - 2, (int)(z + radius));

            for (int ix = minX; ix <= maxX; ++ix) {
                for (int iy = minY; iy <= maxY; ++iy) {
                    for (int iz = minZ; iz <= maxZ; ++iz) {
                        float dx = ix - x, dy = iy - y, dz = iz - z;
                        if (dx*dx + dy*dy*2.0f + dz*dz < radius*radius) {
                            int idx = (iy * height + iz) * width + ix;
                            if (blocks[idx] == Tile::rock->id)
                                blocks[idx] = tileId;
                        }
                    }
                }
            }
        }
    }
}

void LevelGen::floodFill(int x, int y, int z, uint8_t targetBlock) {
    std::vector<int> stack;
    stack.reserve(width * height * 2);
    stack.push_back((y * height + z) * width + x);

    while (!stack.empty()) {
        int idx = stack.back();
        stack.pop_back();
        if (blocks[idx] != 0) continue;
        blocks[idx] = targetBlock;

        int ix = idx % width;
        int temp = idx / width;
        int iz = temp % height;
        int iy = temp / height;

        if (ix > 0)          stack.push_back(idx - 1);
        if (ix < width - 1)  stack.push_back(idx + 1);
        if (iz > 0)          stack.push_back(idx - width);
        if (iz < height - 1) stack.push_back(idx + width);
        if (iy > 0)          stack.push_back(idx - width * height);
    }
}

void LevelGen::addWaterAndLava() {
    const int waterLevel = depth / 2;
    for (int x = 0; x < width; ++x) {
        floodFill(x, waterLevel - 1, 0, Tile::calmWater->id);
        floodFill(x, waterLevel - 1, height - 1, Tile::calmWater->id);
    }
    for (int z = 0; z < height; ++z) {
        floodFill(0, waterLevel - 1, z, Tile::calmWater->id);
        floodFill(width - 1, waterLevel - 1, z, Tile::calmWater->id);
    }

    int waterSources = width * height / 8000;
    for (int i = 0; i < waterSources; ++i) {
        listener->levelLoadProgress(SAFE_DIV(i, waterSources));
        int wx = random.nextInt(width);
        int wy = waterLevel - 1 - random.nextInt(2);
        int wz = random.nextInt(height);
        if (blocks[(wy * height + wz) * width + wx] == 0)
            floodFill(wx, wy, wz, Tile::water->id);
    }

    listener->levelLoadUpdate("Melting..");
    int lavaSources = width * height * depth / 20000;
    for (int i = 0; i < lavaSources; ++i) {
        listener->levelLoadProgress(SAFE_DIV(i, lavaSources));
        int lx = random.nextInt(width);
        int ly = (int)(random.nextFloat() * random.nextFloat() * (waterLevel - 3));
        int lz = random.nextInt(height);
        if (blocks[(ly * height + lz) * width + lx] == 0)
            floodFill(lx, ly, lz, Tile::lava->id);
    }

    for (int x = 0; x < width; ++x) {
        for (int z = 0; z < height; ++z) {
            int idx = (0 * height + z) * width + x;
            if (blocks[idx] != 0) {
                blocks[idx] = Tile::calmLava->id;
            }
        }
    }

    listener->levelLoadProgress(100);
}

void LevelGen::growSurface(const std::vector<int>& map) {
    OctaveNoise sandNoise(random, 8);
    OctaveNoise gravelNoise(random, 8);
    const int waterLevel = depth / 2;

    for (int x = 0; x < width; ++x) {
        listener->levelLoadProgress(x * 100 / (width - 1));
        for (int z = 0; z < height; ++z) {
            bool placeSand = sandNoise.compute(x, z) > 8.0;
            bool placeGravel = gravelNoise.compute(x, z) > 12.0;
            int surfaceY = map[x + z * width];
            if (surfaceY < 0 || surfaceY >= depth - 1) continue;

            int surfaceIdx = (surfaceY * height + z) * width + x;
            int aboveIdx   = ((surfaceY + 1) * height + z) * width + x;
            int aboveId    = blocks[aboveIdx];

            if ((aboveId == Tile::water->id || aboveId == Tile::calmWater->id) &&
                surfaceY <= waterLevel - 1 && placeGravel) {
                blocks[surfaceIdx] = Tile::gravel->id;
            }
            else if (aboveId == 0) {
                uint8_t topId = Tile::grass->id;
                if (surfaceY <= waterLevel - 1 && placeSand)
                    topId = Tile::sand->id;
                blocks[surfaceIdx] = topId;
            }
        }
    }
}

void LevelGen::addSurfaceFlowers(const std::vector<int>& map) {
    int attempts = width * height / 3000;
    for (int i = 0; i < attempts; ++i) {
        int type = random.nextInt(2);
        listener->levelLoadProgress(SAFE_DIV(i, attempts) / 2);
        int x = random.nextInt(width);
        int z = random.nextInt(height);

        for (int j = 0; j < 10; ++j) {
            int mx = x, mz = z;
            for (int k = 0; k < 5; ++k) {
                mx += random.nextInt(6) - random.nextInt(6);
                mz += random.nextInt(6) - random.nextInt(6);
                if (mx >= 0 && mz >= 0 && mx < width && mz < height) {
                    int surfaceY = map[mx + mz * width] + 1;
                    if (surfaceY <= 1 || surfaceY >= depth) continue;
                    int aboveIdx = (surfaceY * height + mz) * width + mx;
                    int groundIdx = ((surfaceY - 1) * height + mz) * width + mx;
                    if (blocks[aboveIdx] == 0 && blocks[groundIdx] == Tile::grass->id) {
                        blocks[aboveIdx] = (type == 0) ? Tile::redFlower->id : Tile::yellowFlower->id;
                    }
                }
            }
        }
    }
}
void LevelGen::addUndergroundMushrooms(const std::vector<int>& map) {
    int attempts = width * height * depth / 2000;
    int spawned = 0;
    for (int i = 0; i < attempts; ++i) {
        int type = random.nextInt(2);
        listener->levelLoadProgress(SAFE_DIV(i, attempts) / 2 + 50);
        int x = random.nextInt(width);
        int y = random.nextInt(depth);
        int z = random.nextInt(height);

        for (int j = 0; j < 20; ++j) {
            int mx = x, my = y, mz = z;
            for (int k = 0; k < 5; ++k) {
                mx += random.nextInt(6) - random.nextInt(6);
                my += random.nextInt(2) - random.nextInt(2);
                mz += random.nextInt(6) - random.nextInt(6);
                if (mx >= 0 && mz >= 0 && my >= 1 && mx < width && mz < height && my < map[mx + mz * width] - 1) {
                    int idx = (my * height + mz) * width + mx;
                    int belowIdx = ((my - 1) * height + mz) * width + mx;
                    if (blocks[idx] == 0 && blocks[belowIdx] == Tile::rock->id) {
                        blocks[idx] = (type == 0) ? Tile::redMushroom->id : Tile::brownMushroom->id;
                        ++spawned;
                    }
                }
            }
        }
    }
    std::cout << "Added " << spawned << " underground mushrooms" << std::endl;
}

void LevelGen::addFlowersAndMushrooms(const std::vector<int>& map) {

    int patches = width * height / 200;
    for (int i = 0; i < patches; ++i) {
        if (patches > 1) listener->levelLoadProgress(i * 100 / (patches - 1));
        int x = random.nextInt(width);
        int z = random.nextInt(height);
        int y = map[x + z * width];
        if (y < 0 || y + 1 >= depth) continue;
        int groundIdx = (y * height + z) * width + x;
        int aboveIdx  = ((y + 1) * height + z) * width + x;
        if (blocks[groundIdx] == Tile::grass->id && blocks[aboveIdx] == 0) {
            uint8_t flower = random.nextInt(2) == 0 ? Tile::yellowFlower->id : Tile::redFlower->id;
            blocks[aboveIdx] = flower;
        }
    }

    patches = width * height / 500;
    for (int i = 0; i < patches; ++i) {
        if (patches > 1) listener->levelLoadProgress(i * 100 / (patches - 1));
        int x = random.nextInt(width);
        int z = random.nextInt(height);
        int y = map[x + z * width];
        if (y < 0 || y + 1 >= depth) continue;
        int groundIdx = (y * height + z) * width + x;
        int aboveIdx  = ((y + 1) * height + z) * width + x;
        if (blocks[groundIdx] == Tile::grass->id && blocks[aboveIdx] == 0) {
            uint8_t mushroom = random.nextInt(2) == 0 ? Tile::redMushroom->id : Tile::brownMushroom->id;
            blocks[aboveIdx] = mushroom;
        }
    }
    listener->levelLoadProgress(100);
}

void LevelGen::addTrees(Level* level, const std::vector<int>& map) {
    int patches = width * height / 4000;
    for (int i = 0; i < patches; ++i) {
        if (patches > 1) listener->levelLoadProgress(i * 100 / (patches - 1));
        int px = random.nextInt(width);
        int pz = random.nextInt(height);
        for (int attempt = 0; attempt < 20; ++attempt) {
            int treeX = px, treeZ = pz;
            for (int step = 0; step < 20; ++step) {
                treeX += random.nextInt(6) - random.nextInt(6);
                treeZ += random.nextInt(6) - random.nextInt(6);
                if (treeX >= 0 && treeZ >= 0 && treeX < width && treeZ < height) {
                    int treeY = map[treeX + treeZ * width] + 1;
                    if (random.nextInt(4) == 0) {
                        level->maybeGrowTree(treeX, treeY, treeZ);
                    }
                }
            }
        }
    }
    listener->levelLoadProgress(100);
}