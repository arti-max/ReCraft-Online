#include "level/EntityMesh.hpp"
#include "util/Utils.hpp"
#include "util/Logger.hpp"

EntityMesh::EntityMesh(int w, int h, int d) {
    this->width = w / 16;
    this->height = h / 16;
    this->depth = d / 16;

    if (this->width == 0) this->width = 1;
    if (this->height == 0) this->height = 1;
    if (this->depth == 0) this->depth = 1;

    this->grid.clear();
    this->grid.resize(this->width * this->height * this->depth);

    this->all.clear();
    this->tmp.clear();
    this->entitiesToAdd.clear();
}

EntityMesh::~EntityMesh() {
    for (Entity* e : this->all) {
        if (!e->removeExternally)
            delete e;
    }
    this->clear();
    
    // delete slotStart;
    // delete slotEnd;
}

void EntityMesh::clear() {
    this->grid.clear();
    this->grid.resize(this->width * this->height * this->depth);
    this->all.clear();
    this->tmp.clear();
    this->entitiesToAdd.clear();
}

void EntityMesh::addEntity(Entity* e) {
    this->entitiesToAdd.push_back(e);
}

void EntityMesh::removeEntity(Entity* e) {
    bool removed = this->slotStart->init(e->x, e->y, e->z).remove(e);
    if (removed) {
        utils::remove_all(this->all, e);
    }
    delete e;
}

std::vector<Entity*> EntityMesh::getEntities(Entity* ignore, float x0, float y0, float z0, float x1, float y1, float z1, std::vector<Entity*>& result) {
    EntityMeshSlot& ss = this->slotStart->init(x0, y0, z0);
    EntityMeshSlot& se = this->slotEnd->init(x1, y1, z1);

    for (int x = ss.xSlot - 1; x <= se.xSlot + 1; ++x) {
        for (int y = ss.ySlot - 1; y <= se.ySlot + 1; ++y) {
            for (int z = ss.zSlot - 1; z <= se.zSlot + 1; ++z) {
                if (x >= 0 && y >= 0 && z >= 0 && x < this->width && y < this->depth && z < this->height) {
                    std::vector<Entity*> entityList = this->grid[(y * this->height + z) * this->width + x];

                    for (int i = 0; i < entityList.size(); ++i) {
                        Entity* e = entityList[i];
                        if (e != ignore && e->intersects(x0, y0, z0, x1, y1, z1)) {
                            result.push_back(e);
                        }
                    }
                }
            }
        }
    }

    return result;
}

std::vector<Entity*> EntityMesh::getEntities(Entity* ignore, const AABB& box) {
    this->tmp.clear();
    return this->getEntities(ignore, box.x0, box.y0, box.z0, box.x1, box.y1, box.z1, this->tmp);
}

void EntityMesh::render(Vec3D vec, Frustum& frustum, Textures* textures, float partialTicks) {
    for (int x = 0; x < this->width; ++x) {
        float minX = (float)((x << 4) - 2);
        float maxX = (float)(((x + 1) << 4) + 2);

        for (int y = 0; y < this->depth; ++y) {
            float minY = (float)((y << 4) - 2);
            float maxY = (float)(((y + 1) << 4) + 2);

            for (int z = 0; z < this->height; ++z) {
                std::vector<Entity*> entityRow = this->grid[(y * this->height + z) * this->width + x];
                if (entityRow.size() != 0) {
                    float minZ = (float)((z << 4) - 2);
                    float maxZ = (float)(((z + 1) << 4) + 2);

                    bool isVisible = frustum.cubeInFrustum(minX, minY, minZ, maxX, maxY, maxZ);
                    bool isFullyVisible = isVisible && frustum.cubeFullyInFrustum(minX, minY, minZ, maxX, maxY, maxZ);

                    if (isVisible) {
                        for (int i = 0; i < entityRow.size(); ++i) {
                            Entity* e = entityRow[i];
                            if ((isFullyVisible || frustum.isVisible(e->bb)) && e && !e->removed) {
                                if (e->shouldRender(vec)) {
                                    e->render(partialTicks, textures);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void EntityMesh::tickAll() {
    if (!this->entitiesToAdd.empty()) {
        for (Entity* e : this->entitiesToAdd) {
            this->all.push_back(e);
            EntityMeshSlot& slot = this->slotStart->init(e->x, e->y, e->z);
            if (slot.xSlot >= 0 && slot.xSlot < this->width &&
                slot.ySlot >= 0 && slot.ySlot < this->depth &&
                slot.zSlot >= 0 && slot.zSlot < this->height) {
                slot.add(e);
                // Logger::logf(PREFIX_DEBUG, "New Entity spawned at %.1f,%.1f,%.1f!", e->x, e->y, e->z);
            } else {
                Logger::logf(PREFIX_WARNING, "Entity at %.1f,%.1f,%.1f outside mesh grid (%i, %i, %i)\n", e->x, e->y, e->z, this->width, this->depth, this->height);
            }
            e->xo = e->x;
            e->yo = e->y;
            e->zo = e->z;
            e->emesh = this; 
        }   
        this->entitiesToAdd.clear();
    }
    for (int i = 0; i < this->all.size(); ++i) {
        if (this->all[i]) {
            Entity* e = this->all[i]; 
            if (!e->network) {
                e->tick();

                if (e->removed) {
                    this->slotStart->init(e->xo, e->yo, e->zo).remove(e);
                    utils::remove_at(this->all, i);
                    i--;

                    if (!e->removeExternally) {
                        delete e;
                    }
                } else {
                    int oldX = (int)(e->xo / 16.0f);
                    int oldY = (int)(e->yo / 16.0f);
                    int oldZ = (int)(e->zo / 16.0f);
                    int X = (int)(e->x / 16.0f);
                    int Y = (int)(e->y / 16.0f);
                    int Z = (int)(e->z / 16.0f);
                    if (oldX != X || oldY != Y || oldZ != Z) {
                        EntityMeshSlot& s1 = this->slotStart->init(e->xo, e->yo, e->zo);
                        EntityMeshSlot& s2 = this->slotEnd->init(e->x, e->y, e->z);
                        if (s1 != s2) {
                            s1.remove(e);
                            s2.add(e);
                            e->xo = e->x;
                            e->yo = e->y;
                            e->zo = e->z;
                        }

                    }

                }
            }
        }
    }
}

int EntityMesh::getMobCount() {
    int cnt = 0;
    if (this->all.size() <= 0) {
        return 0;
    }
    for (int i = 0; i < this->all.size(); i++) {
        if (this->all[i]) {
        Entity* e = this->all[i];
            if (e->getEntityType() != EntityType::Entity && e != nullptr) {
                cnt++;
            }
        }
    }

    return cnt;
}