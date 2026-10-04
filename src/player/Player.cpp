#include "player/Player.hpp"
#include "util/Keyboard.hpp"
#include "util/Logger.hpp"
#include "level/Level.hpp"
#include <GLFW/glfw3.h>
#include "player/PlayerAI.hpp"
#include "render/Textures.hpp"
#include "CrossCraft.hpp"
#include "model/ModelManager.hpp"

Player::Player(Level* level, Settings* settings) : Mob(level) {
    if (level != nullptr) {
        if (level->player == this) {
            Logger::logf(PREFIX_DEBUG, "Remove old player...\n");
            level->removeEntity(this);
        }
        level->player = this;
        level->addEntity(this);
    }
    this->heightOffset = 1.62f;
    this->settings = settings;
    this->health = 20;
    this->modelName = "human";
    this->rotOffs = 180.0f;
    this->footSize = 0.5f;
    this->ai = new PlayerAI(this);
}

void Player::setKey() {
    keys[KEY_UP] = Keyboard::isKeyDown(this->settings->key_forward->keyCode);
    keys[KEY_DOWN] = Keyboard::isKeyDown(this->settings->key_back->keyCode);
    keys[KEY_LEFT] = Keyboard::isKeyDown(this->settings->key_left->keyCode);
    keys[KEY_RIGHT] = Keyboard::isKeyDown(this->settings->key_right->keyCode);
    keys[KEY_JUMP] = Keyboard::isKeyDown(this->settings->key_jump->keyCode);
}

void Player::render(float partialTicks, Textures* textures) {}

void Player::releaseAllKeys() {
    for (int i = 0; i < 10; ++i) {
        keys[i] = false;
    }
}

void Player::resetPos() {
    this->heightOffset = 1.62;
    this->setSize(0.6f, 1.8f);
    Entity::resetPos();
    if (this->level != nullptr) {
        this->level->player = this;
    }
    this->health = Player::MAX_HEALTH;
    this->deathTime = 0;
}

void Player::aiStep() {
    this->inventory->tick();
    this->oBob = this->bob;
    this->input_xxa = 0.0f;
    this->input_yya = 0.0f;

    if (keys[KEY_UP]) {
        --input_yya;
    }
    if (keys[KEY_DOWN]) {
        ++input_yya;
    }
    if (keys[KEY_LEFT]) {
        --input_xxa;
    }
    if (keys[KEY_RIGHT]) {
        ++input_xxa;
    }
    input_jumping = keys[KEY_JUMP];
    Mob::aiStep();
    float tbob = std::sqrt(this->xd * this->xd + this->zd * this->zd);
    float ttilt = std::atan(-this->yd * 0.2f) * 15.0f;

    if (tbob > 0.1f) {
        tbob = 0.1f;
    }

    if (!this->onGround || this->health <= 0) {
        tbob = 0.0f;
    }

    if (this->onGround || this->health <= 0) {
        tilt = 0.0f;
    }

    this->bob += (tbob - this->bob) * 0.4f;
    this->tilt += (ttilt - this->tilt) * 0.8f;

    std::vector<Entity*> entities = this->level->findEntities(this, this->bb.grow(1.0f, 0.0f, 1.0f));
    if (entities.size() > 0) {
        for (int i = 0; i < entities.size(); ++i) {
            entities[i]->playerTouch(this);
        }
    }
}

void Player::bindTexture(CrossCraft* cc) {
    if (this->newTextureId < 0) {
        if (cc->userData != nullptr) {
            this->newTextureId = cc->textures->loadTextureFromUrl("https://crosscraftweb.ddns.net/skins/" + cc->userData->username + ".png", GL_NEAREST);
        } else {
            this->newTextureId = cc->textures->loadTexture("char", GL_NEAREST);
        }
    }
    glBindTexture(GL_TEXTURE_2D, this->newTextureId);
}

HumanModel* Player::getModel() {
    return (HumanModel*)this->modelManager->getModel(this->modelName);
}

void Player::die(Entity* e) {
    this->dead = true;
    this->removeExternally = true;
    this->setSize(0.2f, 0.2f);
    this->setPos(this->x, this->y, this->z);
    this->yd = 0.1f;
    if (e != nullptr) {
        this->xd = -std::cos((this->hurtDir + this->yRot) * M_PI / 180.0f) * 0.1f;
        this->zd = -std::sin((this->hurtDir + this->yRot) * M_PI / 180.0f) * 0.1f;
    } else {
        this->xd = this->zd = 0.0f;
    }

    this->heightOffset = 0.1f;
}

void Player::awardKillScore(Entity* e, int score) {
    this->score += score;
}

int Player::getScore() {
    return this->score;
}

void Player::hurt(Entity* e, int dmg) {
    if (CrossCraft::instance->gamemode->isSurvival()) {
        Mob::hurt(e, dmg);
    }
}

bool Player::isPlayer() {
    return true;
}

bool Player::isShootable() {
    return true;
}

// void Player::tick() {
//     this->xo = x;
//     this->yo = y;
//     this->zo = z;
    
//     float xa = 0.0f;
//     float ya = 0.0f;
//     bool inWater = this->isInWater();
//     bool inLava = this->isInLava();

//     if (keys[KEY_UP]) {
//         --ya;
//     }
//     if (keys[KEY_DOWN]) {
//         ++ya;
//     }
//     if (keys[KEY_LEFT]) {
//         --xa;
//     }
//     if (keys[KEY_RIGHT]) {
//         ++xa;
//     }
    
//     if (keys[KEY_JUMP]) {
//         if (inWater) {
//             this->yd += 0.04f;
//         } else if (inLava) {
//             this->yd += 0.04f;
//         } else if (onGround) {
//             this->yd = 0.42f;
//         }
//     }
    
//     float oldY;
//     if (inWater) {
//         oldY = this->y;
//         this->moveRelative(xa, ya, 0.02f);
//         this->move(this->xd, this->yd, this->zd);
//         this->xd *= 0.8f;
//         this->yd *= 0.8f;
//         this->zd *= 0.8f;
//         this->yd -= 0.02f;
//         // Logger::logf(PREFIX_DEBUG, "hor: %i, is free: %i", this->horizontalCollision, this->isFree(this->xd, this->yd + 0.6f - this->y + oldY, this->zd));
//         if (this->horizontalCollision && this->isFree(this->xd, this->yd + 0.6f - this->y + oldY, this->zd)) {
//             this->yd = 0.3f;
//         }
//     } else if (inLava) {
//         oldY = this->y;
//         this->moveRelative(xa, ya, 0.02f);
//         this->move(this->xd, this->yd, this->zd);
//         this->xd *= 0.5f;
//         this->yd *= 0.5f;
//         this->zd *= 0.5f;
//         this->yd -= 0.02f;
        
//         if (this->horizontalCollision && this->isFree(this->xd, this->yd + 0.6f - this->y + oldY, this->zd)) {
//             this->yd = 0.3f;
//         }
//     } else {
//         this->moveRelative(xa, ya, this->onGround ? 0.1f : 0.02f);
//         this->move(this->xd, this->yd, this->zd);
        
//         this->xd *= 0.91f;
//         this->yd *= 0.98f;
//         this->zd *= 0.91f;
//         this->yd -= 0.08f;
        
//         if (onGround) {
//             this->xd *= 0.6f;
//             this->zd *= 0.6f;
//         }
//     }

// }

