#include "mob/Sheep.hpp"
#include "Entity.hpp"
#include "GL/gl.h"
#include "Random.hpp"
#include "level/Level.hpp"
#include "item/Item.hpp"
#include "mob/ai/BasicAI.hpp"
#include "level/tile/Tile.hpp"
#include "model/SheepFurModel.hpp"
#include "model/SheepModel.hpp"
#include "player/Player.hpp"

Sheep::Sheep(Level* level, float x, float y, float z) : Mob(level) {
    this->setSize(1.4f, 1.72f);
    this->setPos(x, y, z);
    this->heightOffset = 1.72f;
    this->modelName = "sheep";
    this->textureName = "/mob/sheep.png";
    this->ai = new BasicAI();
}

void Sheep::aiStep() {
    Mob::aiStep();
}

void Sheep::die(Entity* e) {
    if (e != nullptr) {
        e->awardKillScore(this, 10);
    }

    int cnt = this->level->random->nextInt(2)+1;
    for (int i = 0; i < cnt; i++) {
        this->level->addEntity((Entity*)new Item(this->level, this->x, this->y, this->z, Tile::brownMushroom->id));
    }

    Mob::die(e);
}

void Sheep::hurt(Entity* e, int dmg) {
    if (this->hasFur && dynamic_cast<Player*>(e)) {
        this->hasFur = false;
        int cnt = (int)(Random::random() * 3.0f + 1.0f);
        for (int i = 0; i < cnt; ++i) {
            this->level->addEntity(new Item(this->level, this->x, this->y, this->z, Tile::wool16->id));
        }
    } else {
        Mob::hurt(e, dmg);
    }
}

void Sheep::renderModel(Textures* textures, float time, float speed, float tick, float headYRot, float headXRot, float scale) {
    SheepModel* model = (SheepModel*)this->modelManager->getModel(this->modelName);
    float headY = model->head->y;
    float headZ = model->head->z;
    Mob::renderModel(textures, time, speed, tick, headYRot, headXRot, scale);
    if (this->hasFur) {
        glBindTexture(GL_TEXTURE_2D, textures->loadTexture("/mob/sheep_fur.png", GL_NEAREST));
        glDisable(GL_CULL_FACE);
        SheepFurModel* fur = (SheepFurModel*)this->modelManager->getModel("sheep.fur");
        fur->head->pitch = model->head->pitch;
        fur->head->x = model->head->x;
        fur->head->y = model->head->y;
        fur->body->yaw = model->body->yaw;
        fur->body->pitch = model->body->pitch;
        fur->leg1->pitch = model->leg1->pitch;
        fur->leg2->pitch = model->leg2->pitch;
        fur->leg3->pitch = model->leg3->pitch;
        fur->leg4->pitch = model->leg4->pitch;
        fur->head->render(scale);
        fur->body->render(scale);
        fur->leg1->render(scale);
        fur->leg2->render(scale);
        fur->leg3->render(scale);
        fur->leg4->render(scale);
    }
    model->head->y = headY;
    model->head->z = headZ;
}