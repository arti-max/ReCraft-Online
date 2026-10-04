#pragma once
#include "Entity.hpp"
#include "render/Textures.hpp"
#include "render/Tessellator.hpp"
#include "item/ItemModel.hpp"
#include <vector>

class Item : public Entity {
private:
    int resourceId = 0;
    int age = 0;
    float hoverAnim = 0.0f;
    int rot = 0;

    float xd = 0.0f;
    float yd = 0.0f;
    float zd = 0.0f;
public:
    static std::vector<ItemModel*> models; 
    Item(Level* level, float x, float y, float z, int resourceId);
    virtual ~Item() = default;

    void tick() override;
    void render(float partialTicks, Textures* textures) override;
    void renderFace(Tessellator& t, int x, int y, int z, int face);
    void playerTouch(Player* p) override;
    int getResourceId();
    static void initModels(); 
};