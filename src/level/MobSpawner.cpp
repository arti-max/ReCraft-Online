#include "level/MobSpawner.hpp"

#include "Logger.hpp"
#include "level/Level.hpp"
#include "Entity.hpp"
#include "level/LevelLoaderListener.hpp"
#include "level/tile/Tile.hpp"
// mobs:
#include "mob/Sheep.hpp"
#include "mob/Spider.hpp"
#include "mob/Zombie.hpp"
#include "mob/Skeleton.hpp"
#include "mob/Creeper.hpp"
#include "mob/AnimalMob.hpp"
#include "mob/HumanMob.hpp"

MobSpawner::MobSpawner(Level* level) : level(level) {}

void MobSpawner::spawn(int area, Entity* player, LevelLoaderListener* listener) {
    int spawned = 0;
    // Logger::logf(PREFIX_DEBUG, "Before area cycle!\n");
    for (int i = 0; i < area; i++) {
        if (area > 1 && listener != nullptr) {
            // Logger::logf(PREFIX_DEBUG, "Before load progress!\n");
            listener->levelLoadProgress(i * 100 / (area - 1));
            // Logger::logf(PREFIX_DEBUG, "After load progress!\n");
        }
        
        if (this->level->random != nullptr) {
            // Logger::logf(PREFIX_DEBUG, "random is nullptr...!\n");
        }
        int type = this->level->random->nextInt(6);
        // Logger::logf(PREFIX_DEBUG, "After type randomed!\n");
        int spawnX = this->level->random->nextInt(this->level->width);
        int spawnY = std::min(this->level->random->nextFloat(), this->level->random->nextFloat()) * this->level->depth;
        // Logger::logf(PREFIX_DEBUG, "After y coord!\n");
        int spawnZ = this->level->random->nextInt(this->level->height);
        
        // Logger::logf(PREFIX_DEBUG, "Before borders check!\n");
        if (spawnX < 0 || spawnY < 0 ||spawnZ < 0 || spawnX >= this->level->width || spawnY >= this->level->depth || spawnZ >= this->level->height) {
            continue;
        }

        if (this->level->isSolidTile(spawnX, spawnY, spawnZ)) {
            continue;
        }

        int tileId = this->level->getTile(spawnX, spawnY, spawnZ);
        Tile* tile = Tile::tiles[tileId];
        if (tile && tile->getLiquidType() != LiquidType::NOT_LIQUID) {
            continue;
        }

        // Logger::logf(PREFIX_DEBUG, "Before check lit and grouping!\n");
        if (!level->isLit(spawnX, spawnY, spawnZ) || this->level->random->nextInt(5) == 0) {
            for (int attempt = 0; attempt < 3; ++attempt) {
                int mx = spawnX;
                int my = spawnY;
                int mz = spawnZ;
                for (int step = 0; step < 3; ++step) {
                    mx += level->random->nextInt(6) - level->random->nextInt(6);
                    my += level->random->nextInt(1) - level->random->nextInt(1);
                    mz += level->random->nextInt(6) - level->random->nextInt(6);

                    if (mx < 0 || my < 0 || mz < 0 || mx >= level->width || my >= level->depth || mz >= level->height) {
                        continue;
                    }

                    if (level->isSolidTile(mx, my-1, mz) && !level->isSolidTile(mx, my, mz) && !level->isSolidTile(mx, my+1, mz)) {
                        float fx = mx + 0.5f;
                        float fy = my + 1.0f;
                        float fz = mz + 0.5f;
                        float dx = 0;
                        float dy = 0;
                        float dz = 0;
                        if (player != nullptr) {
                            dx = fx - player->x;
                            dy = fy - player->y;
                            dz = fz - player->z;
                            if (dx*dx + dy*dy + dz*dz < 256.0f) {
                                continue;
                            }
                        } else {
                            dx = fx - level->xSpawn;
                            dy = fy - level->ySpawn;
                            dz = fz - level->zSpawn;
                            if (dx*dx + dy*dy + dz*dz < 256.0f) {
                                continue;
                            }
                        }

                        Entity* mob = nullptr;
                        switch (type) {
                            case 0: mob = new Zombie(level, fx, fy, fz); break;
                            case 1: mob = new Skeleton(level, fx, fy, fz); break;
                            case 2: mob = new AnimalMob(level, fx, fy, fz); break;
                            case 3: mob = new Creeper(level, fx, fy, fz); break;
                            case 4: mob = new Spider(level, fx, fy, fz); break;
                            case 5: mob = new Sheep(level, fx, fy, fz); break;
                        }

                        if (mob) {
                            AABB bb = ((Mob*)mob)->bb;
                            if (level->isFree(bb)) {
                                ++spawned;
                                level->addEntity(mob);
                            } else {
                                delete mob;
                            }
                        }
                    }
                }

            }
        }
    }

}