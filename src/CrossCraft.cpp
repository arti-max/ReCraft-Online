#include "CrossCraft.hpp"
#include <malloc.h>
#include <emscripten/heap.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <gc.h>
#include "Logger.hpp"
#include "emscripten/emscripten.h"
#include "gamemode/SurvivalGameMode.hpp"
#include "gc/gc.h"
#include "mob/Spider.hpp"
#include "model/Vec3D.hpp"
#include "model/ModelPart.hpp"
#include "mob/AnimalMob.hpp"
#include "net/packet/BlockChangePacket.hpp"

CrossCraft* CrossCraft::instance = nullptr;

CrossCraft::CrossCraft(const char* canvas, int w, int h, bool fs) : 
    width(w), 
    height(h), 
    fullscreen(fs), 
    window(nullptr),
    textures()
{
    CrossCraft::instance = this;
    if (canvas) {
        parent = canvas;
    }

    this->textures = new Textures();
    this->sound = new SoundManager();

    textureEffects.push_back(new WaterTextureFX());
    textureEffects.push_back(new LavaTextureFX());
    
    GC_add_roots(this, (char*)this + sizeof(*this));
}

CrossCraft::~CrossCraft() {
    this->destroy();
    if (this->textures) {
        delete this->textures;
    }
    if (this->level) {
        delete this->level;
    }
}
 
void CrossCraft::destroy() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
    Mouse::destroy();
    Keyboard::destroy();
    Logger::logf(PREFIX_CC, "CrossCraft destroyed.\n");
}

void CrossCraft::init() {
    int col1 = 920330;
    this->bgB = 0.92f;
    this->bgG = 0.98f;
    this->bgB = 1.0f;
    fogColor0 = {this->bgR, this->bgG, this->bgB, 1.0};
    fogColor1 = {(float)14/255.0f, (float)11/255.0f, (float)10/255.0f, 1.0f};


    if (!glfwInit()) {
        Logger::logf(PREFIX_ERROR, "Failed to initialize GLFW\n");
    }

    glfwWindowHint(GLFW_ALPHA_BITS, 0);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    const char* canvas = parent.empty() ? nullptr : parent.c_str();

    window = glfwCreateWindow(width, height, "CrossCraft", NULL, NULL);
    if (!window) {
        Logger::logf(PREFIX_ERROR, "Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    Keyboard::create();
    Mouse::create();
    this->checkGlError("Pre startup");
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glCullFace(GL_BACK);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    this->checkGlError("Startup");
    this->sound->initOpenAL(this->settings);
    this->font = new Font("default", this->textures);
    this->chatGui = new ChatGui();
    this->hud = new Hud(this, this->textures, this->width*240/this->height, this->height*240/this->height);
    glViewport(0, 0, this->width, this->height);
    this->level = new Level();
    this->level->cc = this;
    this->player = new Player(this->level, this->settings);
    this->particleEngine = new ParticleEngine(this->level);
    this->progressbar = new Progressbar(this->width, this->height, this->textures);

    if (!this->mpMode) {
        if (this->loadMapUser.empty() || this->loadMapId == -1) {
            if (this->userData != nullptr) {
                this->levelGen->generateLevel(this->level, this->userData->username.c_str(), 256, 256, 64);
            } else {
                this->levelGen->generateLevel(this->level, "noname", 256, 256, 64);
            }
        } else {
            this->loadLevel(this->loadMapUser.c_str(), this->loadMapId);
        }
    } else {
        this->connectionUrls.push_back("ws://" + this->serverAddress + ":" + std::to_string(this->serverPort));
        
        this->connectionUrls.push_back("ws://" + this->serverAddress + "/ws/");

        this->currentUrlIndex = 0;
        this->connectToServer();
    }

    this->levelRenderer = new LevelRenderer(this->level, this->textures);
    this->gamemode->preparePlayer(this->player);
    this->player->resetPos();
    this->gamemode->apply(this->level);
    this->gamemode->prepareLevel(this->level);
    // this->level->player = this->player;

    // if (this->level != nullptr) {
    //     this->level->player = this->player;
    //     this->level->addEntity(this->player);
    // }

    Mouse::init(window);
    Keyboard::init(window);
    Item::initModels();
    Data::initAllowedTiles();

    this->gamemode->apply(this->player);

    Keyboard::enableRepeatEvents(false);

    this->checkGlError("Post startup");
    
}

void CrossCraft::setScreen(Screen* screen) {
    if (this->screen != nullptr) this->screen->onClose();
    if (this->screen != nullptr && screen == nullptr) {
        delete this->screen;
    }

    if (screen == nullptr && this->player->health <= 0) {
        screen = new DeathScreen();
    }

    if (this->screen == nullptr && screen != nullptr) {
        Keyboard::clearEvents();
        Mouse::clearEvents();
    }
    this->screen = screen;
    if (screen == nullptr) {
        Mouse::clearEvents();
        this->clickDelay = 10;
    }
    if (screen != nullptr) {
        int screenWidth = this->width * 240 / this->height;
        int screenHeight = this->height * 240 / this->height;
        screen->init(this, screenWidth, screenHeight);
        // this->screenStack.push_back(screen);
    }
}

void CrossCraft::grabMouse() {
    if (!this->mouseGrabbed) {
        Logger::logf(PREFIX_DEBUG, "CrossCraft: Requesting mouse grab\n");
        Mouse::setGrabbed(true);
        if (!this->appletMode) {
            Mouse::setCursorPosition(width / 2, height / 2);
        }
        this->setScreen(nullptr);
    }
}

void CrossCraft::releaseMouse() {
    if (this->mouseGrabbed) {
        Logger::logf(PREFIX_DEBUG, "CrossCraft: Releasing mouse\n");
        Mouse::setGrabbed(false);
        this->player->releaseAllKeys();
        if (this->screen == nullptr) {
            this->setScreen((Screen*)(new PauseScreen()));
        }
    }
}
 
void CrossCraft::stop() {
    this->running = false;
}

void CrossCraft::run() {
    if (this->userData != nullptr) {
        Logger::logf(PREFIX_CC, "CrossCraft started! canvas=%s, size=%dx%d, user=%s\n", 
            this->parent.c_str(), 
            this->width, 
            this->height, 
            this->userData->username.c_str());
    } else {
        Logger::logf(PREFIX_CC, "CrossCraft started! canvas=%s, size=%dx%d, guest mode\n", 
            this->parent.c_str(), 
            this->width, 
            this->height);
    }
    try {
        this->init();
    } catch (const std::exception& e) {
        Logger::logf(PREFIX_ERROR, "Failed to start CrossCraft: %s\n", e.what());
        return;
    }

    this->running = true;
    this->lastFpsTime = emscripten_get_now();

    while (this->running) {
        this->mainLoop();

#ifdef __EMSCRIPTEN__
        emscripten_sleep(0);
#endif
    }

    this->destroy();
}

void CrossCraft::mainLoop() {
    if (paused) {
        return;
    }

    if (glfwWindowShouldClose(window)) {
        this->stop();
        return;
    }

    if (this->mpMode && this->client != nullptr) {
        while (!this->client->incomingPackets.empty()) {
            Packet* packet = this->client->incomingPackets.front();
            this->client->incomingPackets.pop();

            this->netData->handler->handleNetworkPacket(packet);

            delete packet;
        }
    }
    
    this->timer->advanceTime();

    for (int i = 0; i < this->timer->ticks; ++i) {
        this->tick();
    }

    if (this->inErrorState) {
        this->drawErrorScreen();
    } else if (this->canRender) {
        this->checkGlError("Pre render");
        this->render(this->timer->partialTicks);
        this->checkGlError("Post render");
    }
    ++this->frames;
    glfwPollEvents();

    double now = emscripten_get_now();
    if (now >= this->lastFpsTime + 1000.0) {
        this->fpsString = std::to_string(this->frames) + " fps, " + std::to_string(Chunk::updates) + " chunk updates";
        // Logger::logf(PREFIX_DEBUG, "%s\n", this->fpsString.c_str());
        Chunk::updates = 0;
        this->frames = 0;
        this->lastFpsTime += 1000.0;
    }
}

bool CrossCraft::isFree(const AABB &aabb) {
    if (this->player->bb.intersects(aabb)) {
        return false;
    } else {
        for (int i = 0; i < this->level->entities.size(); ++i) {
            if (this->level->entities[i]->bb.intersects(aabb)) {
                return false;
            }
        }

        return true;
    }
}

void CrossCraft::handleMouseClick(int mode) {
    if (!(mode != 0 || this->attackTime <= 0)) {
        return;
    }
    if (mode == 0) {
        this->heldBlock->offset = -1;
        this->heldBlock->moving = true;
    }

    int selected = this->player->inventory->getCurrentBlock();
    if (mode == 1 && selected > 0 && this->gamemode->useItem(this->player, selected)) {
        this->heldBlock->pos = 0.0f;
    } else if (this->hitResult == nullptr) {
        if (this->gamemode->gmType == 0 && mode == 0) {
            this->attackTime = 10;
        }
    } else {
        if (this->hitResult->entityPos == 1) {
            if (mode == 0) {
                this->hitResult->e->hurt(this->player, 4);
                return;
            }
        } else if (this->hitResult->entityPos == 0) {
            int x = this->hitResult->x;
            int y = this->hitResult->y;
            int z = this->hitResult->z;

            if (mode != 0) {
                if (this->hitResult->f == 0) y--;
                if (this->hitResult->f == 1) y++;
                if (this->hitResult->f == 2) z--;
                if (this->hitResult->f == 3) z++;
                if (this->hitResult->f == 4) x--;
                if (this->hitResult->f == 5) x++;
            }

            Tile* tile = Tile::tiles[this->level->getTile(x, y, z)];
            if (mode == 0) {
                if (tile->id != Tile::unbreakable->id) {
                    this->gamemode->hitTile(x, y, z);
                    return;
                }
            } else { 
                int selected = this->player->inventory->getCurrentBlock();
                if (selected <= 0) {
                    return;
                }

                Tile* tile2 = Tile::tiles[this->level->getTile(x, y, z)];
                AABB* tilebb = Tile::tiles[selected]->getAABB(x, y, z);
                if ((tile2 == nullptr || tile2->id == Tile::water->id || tile2->id == Tile::calmWater->id || tile2->id == Tile::lava->id || tile2->id == Tile::calmLava->id) && (tilebb == nullptr || (this->player->intersects(tilebb->x0, tilebb->y0, tilebb->z0, tilebb->x1, tilebb->y1, tilebb->z1) ? false : this->isFree(AABB(tilebb->x0, tilebb->y0, tilebb->z0, tilebb->x1, tilebb->y1, tilebb->z1))))) {
                    if (!this->gamemode->canPlace(selected)) {
                        return;
                    }

                    this->level->setTile(x, y, z, selected);
                    this->heldBlock->pos = 0.0f;

                    if (this->mpMode) {
                        BlockChangePacket* packet = new BlockChangePacket(x, y, z, selected);
                        this->client->sendPacket(packet);
                    }
                }
            }
        }
    }
    // if (mode == 0) {
    //     if (this->hitResult != nullptr) {

    //         // Tile* previousTile = Tile::tiles[this->level->getTile(this->hitResult->x, this->hitResult->y, this->hitResult->z)];

    //         // bool tileChanged = this->level->setTile(this->hitResult->x, this->hitResult->y, this->hitResult->z, 0);
    //         // if (previousTile != nullptr && tileChanged) {
    //         //     bool particles = true;
    //         //     if (previousTile->id == Tile::unbreakable->id) {
    //         //         particles = false;
    //         //         this->level->setTile(this->hitResult->x, this->hitResult->y, this->hitResult->z, previousTile->id);
    //         //     }
    //         //     if (this->mpMode && this->client && this->client->isConnected()) {
    //         //         // this->level->setTile(this->hitResult->x, this->hitResult->y, this->hitResult->z, previousTile->id);
    //         //         BlockChangePacket* packet = new BlockChangePacket(
    //         //             this->hitResult->x, this->hitResult->y, this->hitResult->z, 
    //         //             0, false);
    //         //         client->sendPacket(packet);
    //         //     }
    //         //     if ((previousTile->st != &SoundType::none) && particles) {
    //         //         this->level->playSound("step." + previousTile->st->name, (float)this->hitResult->x, (float)this->hitResult->y, (float)this->hitResult->z, (previousTile->st->getVolume() + 1.0f) / 2.0f, previousTile->st->getPitch() * 0.8f);
    //         //     }
    //         //     // if (Data::survival) {
    //         //     //     this->player->inventory->addItem(previousTile->id);
    //         //     // }
    //         //     if (particles) previousTile->onDestroy(this->level, this->hitResult->x, this->hitResult->y, this->hitResult->z, this->particleEngine, Data::survival);
    //         // }
    //     }
    // } else if (this->hitResult != nullptr && mode == 1) {
    //     int x = this->hitResult->x;
    //     int y = this->hitResult->y;
    //     int z = this->hitResult->z;

    //     if (this->hitResult->f == 0) y--;
    //     if (this->hitResult->f == 1) y++;
    //     if (this->hitResult->f == 2) z--;
    //     if (this->hitResult->f == 3) z++;
    //     if (this->hitResult->f == 4) x--;
    //     if (this->hitResult->f == 5) x++;

    //     if (this->player->inventory->getCurrentBlock() != -1) {
    //         AABB* aabb = Tile::tiles[this->player->inventory->getCurrentBlock()]->getAABB(x, y, z);
        
    //         if (aabb == nullptr || this->isFree(*aabb)) {
    //             this->level->setTile(x, y, z, this->player->inventory->getCurrentBlock());
    //             if (this->mpMode && client && client->isConnected()) {
    //                 BlockChangePacket* packet = new BlockChangePacket(
    //                         x, y, z, 
    //                         this->player->inventory->getCurrentBlock(), true);
    //                 client->sendPacket(packet);
    //             }
    //             if (Data::survival) {
    //                 this->player->inventory->removeItem(this->player->inventory->getCurrentBlock());
    //             }
    //         }
            
    //         if (aabb != nullptr) {
    //             delete aabb;
    //         }
    //     }
    // }
}

void CrossCraft::tick() {
    if (this->sound != nullptr) {
        this->sound->tick();
    }

    if (this->settings) {
        this->levelRenderer->drawDistance = this->settings->renderDistance;
    }

    if (this->attackTime > 0) {
        this->attackTime--;
    }
    if (this->clickDelay > 0) {
        this->clickDelay--;
    }

    bool isActuallyGrabbed = Mouse::isGrabbed();

    if (this->screen == nullptr && this->player != nullptr && this->player->health <= 0) {
        this->setScreen((Screen*)nullptr);
    }

    if (this->mouseGrabbed && !isActuallyGrabbed) {
        printf("CrossCraft: Pointer lock lost\n");
        this->mouseGrabbed = false;
        this->player->releaseAllKeys();
        
        if (this->screen == nullptr) {
            this->setScreen(new PauseScreen());
        }
    } 
    
    else if (!this->mouseGrabbed && isActuallyGrabbed) {
        printf("CrossCraft: Pointer lock acquired\n");
        this->mouseGrabbed = true;
        
        if (this->screen != nullptr && dynamic_cast<PauseScreen*>(this->screen)) {
            this->setScreen(nullptr);
        }
    }

    this->chatGui->tick();

    if (this->waitingForFocus) {
        if (Mouse::getEventButtonState()) {
            this->grabMouse();
        }
        
        if (Mouse::isGrabbed()) {
            this->waitingForFocus = false;
            this->mouseGrabbed = true;
        }
        
        return; 
    }

    if (this->mouseGrabbed && !Mouse::isGrabbed()) {
        printf("CrossCraft: Pointer lock released by browser (probably ESC)\n");
        this->releaseMouse();
    }

    if (this->screen == nullptr) {
        this->raycast();
    }

    if (this->screen == nullptr || this->screen->grabMouse) {
        while (Keyboard::next()) {
            if (this->player != nullptr) {
                this->player->setKey();
            }
            if (Keyboard::getEventKeyState()) {
                if (this->mpMode) {
                    playerListScreen->updateKeyboardEvents();
                }
                if (this->screen != nullptr) {
                    this->screen->updateKeyboardEvents();
                }

                if (Keyboard::getEventKey() == GLFW_KEY_ESCAPE) {
                    this->releaseMouse();
                    break;
                }

                if (Keyboard::getEventKey() == this->settings->key_chat->keyCode && this->mpMode == true) {
                    this->player->releaseAllKeys();
                    this->setScreen(new ChatScreen());
                    this->releaseMouse();
                    break;
                }

                if (Keyboard::getEventKey() == GLFW_KEY_TAB ) {
                    if (this->gamemode->isSurvival() && (this->player->inventory->removeArrow()))
                        this->level->addEntity(new Arrow(this->level, this->player, this->player->x, this->player->y, this->player->z, this->player->yRot, this->player->xRot, 1.2f));
                }

                if (Keyboard::getEventKey() == this->settings->key_build->keyCode) {
                    this->gamemode->openInventory();
                    break;
                    // this->player->releaseAllKeys();
                    // this->setScreen((Screen*)(new InventoryScreen()));
                    // this->releaseMouse();
                    // break;
                    // this->level->addEntity(new Spider(this->level, this->player->x, this->player->y, this->player->z));
                }

                if (Keyboard::getEventKey() == this->settings->key_save->keyCode) {
                    this->level->setSpawnPos((int)this->player->x, (int)this->player->y, (int)this->player->z, (int)this->player->yRot);
                    this->player->resetPos();
                }

                if (Keyboard::getEventKey() == this->settings->key_load->keyCode && !this->gamemode->isSurvival()) {
                    this->player->resetPos();
                }

                if (Keyboard::getEventKey() == GLFW_KEY_F5) {
                    this->raining = !this->raining;
                }

                if (Keyboard::getEventKey() >= GLFW_KEY_1 && Keyboard::getEventKey() <= GLFW_KEY_9) {
                    int keyIndex = Keyboard::getEventKey() - GLFW_KEY_1;
                    if (keyIndex < this->player->inventory->slots.size()) {
                        this->player->inventory->selectedSlot = keyIndex;
                        // this->selectedTile = this->hotbarSlots[this->hotbarIndex];
                    }
                }
                if (Keyboard::getEventKey() == this->settings->key_fog->keyCode) {
                    this->settings->toggleSetting(4, 1);
                }
                if (Keyboard::getEventKey() == GLFW_KEY_G && !this->mpMode && this->level->entities.size() < 256) {
                    // if (Random::random() < 0.5f) {
                    //     this->level->addEntity((Entity*)new Zombie(this->level, this->player->x, this->player->y, this->player->z));
                    // } else if (Random::random() < 0.5f){
                    //     this->level->addEntity((Entity*)new Skeleton(this->level, this->player->x, this->player->y, this->player->z));
                    // } else {
                    //     this->level->addEntity((Entity*)new AnimalMob(this->level, this->player->x, this->player->y, this->player->z));
                    // }
                }
            }
        }
        double dWheel = Mouse::getDWheel();
        if (dWheel != 0.0) {
            const double threshold = 5.0; 
            
            int scrollDirection = 0;
            if (dWheel > threshold) scrollDirection = 1;
            else if (dWheel < -threshold) scrollDirection = -1;

            if (scrollDirection != 0 && this->player != nullptr) {
                int steps = static_cast<int>(dWheel / 100.0);
                if (steps == 0) steps = scrollDirection;

                if (this->player->inventory != nullptr) {
                    this->player->inventory->selectedSlot += steps;
                    int numSlots = this->player->inventory->slots.size();
                    this->player->inventory->selectedSlot = (this->player->inventory->selectedSlot % numSlots + numSlots) % numSlots;
                    // this->selectedTile = this->hotbarSlots[this->hotbarIndex];
                }
            }
        }
        while (Mouse::next()) {
            if (this->screen == nullptr) {
                if (!this->mouseGrabbed && Mouse::getEventButtonState()) {
                    this->grabMouse();
                    break;
                }

                if (!this->mouseGrabbed && Mouse::getEventButtonState()) {
                    this->grabMouse();
                } else {
                    if (Mouse::getEventButton() == 0 && Mouse::getEventButtonState()) {
                        this->handleMouseClick(0);
                        this->lastClick = this->ticks;
                        // this->attackTime = 5;
                    }

                    if (Mouse::getEventButton() == 2 && Mouse::getEventButtonState()) {
                        this->handleMouseClick(1);
                        this->lastClick = this->ticks;
                    }
                    if (Mouse::getEventButton() == 1 && Mouse::getEventButtonState()) {
                        if (this->hitResult != nullptr) {
                            int pickedID = this->level->getTile(this->hitResult->x, this->hitResult->y, this->hitResult->z);
                            
                            if (pickedID == Tile::grass->id) {
                                pickedID = Tile::dirt->id;
                            }

                            if (pickedID == Tile::unbreakable->id) {
                                pickedID = Tile::rock->id;
                            }

                            if (this->player->inventory->inInventory(pickedID) != -1 || !this->gamemode->isSurvival()) {
                                this->player->inventory->pickTile(pickedID);
                            } 

                            // if (pickedID > 0 && Tile::tiles[pickedID] != nullptr && Tile::tiles[pickedID]->mayPick()) {
                            //     for (int i = 0; i < this->player->inventory->slots.size(); ++i) {
                            //         if (this->player->inventory->slots[i] == pickedID) {
                            //             this->player->inventory->selectedSlot = i;
                            //             // this->selectedTile = this->hotbarSlots[this->hotbarIndex];
                            //             break;
                            //         }
                            //     }
                            // }
                        }
                    }
                }
            }

            if (this->mpMode) {
                playerListScreen->updateMouseEvents();
            }
            if (this->screen != nullptr) {
                this->screen->updateMouseEvents();
            }
        }

        if (this->screen == nullptr) {
            if (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && ((float)(this->ticks - this->lastClick) >= this->timer->ticksPerSecond / 4.0f)) {
                this->handleMouseClick(0);
                this->lastClick = this->ticks;
            }
            if (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && ((float)(this->ticks - this->lastClick) >= this->timer->ticksPerSecond / 4.0f)) {
                this->handleMouseClick(1);
                this->lastClick = this->ticks;
            }
        }

        if (!this->gamemode->instantBreak && this->attackTime <= 0) {
            if (this->screen == nullptr && this->hitResult != nullptr && this->hitResult->entityPos == 0 && glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                int x = this->hitResult->x;
                int y = this->hitResult->y;
                int z = this->hitResult->z;
                this->gamemode->hitTile(x, y, z, this->hitResult->f);
            } else {
                this->gamemode->resetHits();
            }
        }

        // if (this->screen == nullptr && glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        //     if (this->attackTime <= 0) {
        //         this->handleMouseClick(0);
        //         // this->attackTime = 5; 
        //     }
        // }

    }

    if (this->screen != nullptr) {
        this->lastClick = this->ticks + 10000;
    }

    if (this->screen != nullptr) {
        while (Mouse::next()) {
            this->screen->updateMouseEvents();
            if (this->mpMode) {
                this->playerListScreen->updateMouseEvents();
            }
        }
        while (Keyboard::next()) {
            this->screen->updateKeyboardEvents();
            if (this->mpMode) {
                this->playerListScreen->updateKeyboardEvents();
            }
        }
    }

    if (this->screen != nullptr) {
        this->screen->tick();
    }

    if (this->mpMode) {
        if (playerListScreen != nullptr) {
            playerListScreen->tick();
        }
    }

update_world:
    for (TextureFX* fx : textureEffects) {
        fx->tick();
        textures->updateTextureFX(fx->pixels, fx->textureId);
    }
    if (!this->mpMode && this->gamemode->isSurvival()) {
        this->gamemode->spawnMob();
    }
    ++this->levelRenderer->cloudTicks;
    ++this->hud->ticks;
    this->level->tick();
    this->particleEngine->tick();

    if (this->raining) {
        int x = (int)this->player->x;
        int y = (int)this->player->y;
        int z = (int)this->player->z;

        for (int i = 0; i < 50; ++i) {
            int px = x + this->level->random->nextInt(9) - 4;
            int pz = z + this->level->random->nextInt(9) - 4;
            int py = this->level->getHighestTile(px, pz);
            if (py <= y + 4 && py >= y - 4) {
                float mx = this->level->random->nextFloat();
                float mz = this->level->random->nextFloat();
                this->particleEngine->add(new WaterDropPatricle(this->level, px + mx, py + 0.1f, pz + mz));
            }
        }
    }

    if (this->level != nullptr) {
        this->heldBlock->lastPos = this->heldBlock->pos;
        if (this->heldBlock->moving) {
            ++this->heldBlock->offset;
            if (this->heldBlock->offset == 7) {
                this->heldBlock->offset = 0;
                this->heldBlock->moving = false;
            }
        }

        if (this->player != nullptr) {
            if (this->player->inventory != nullptr) {
                int selected = this->player->inventory->getCurrentBlock();
                Tile* tile = nullptr;
                if (selected > 0) {
                    tile = Tile::tiles[selected];
                }

                float maxStep = 0.4f;
                float step = 0.0f;
                if (tile != nullptr) {
                    step = (tile->id == this->heldBlock->tile->id ? 1.0f : 0.0f) - this->heldBlock->pos;
                } else {
                    step = (this->heldBlock->tile == nullptr ? 1.0f : 0.0f) - this->heldBlock->pos;
                }
                if (step < -maxStep) step = -maxStep;
                if (step > maxStep) step = maxStep;
                this->heldBlock->pos += step;
                if (this->heldBlock->pos < 0.1f) {
                    this->heldBlock->tile = tile;
                }
            }
        }
    }

    // this->player->tick();
    // this->player->inventory->tick();
    this->sound->updateListener(this->player->x, this->player->y+1.62, this->player->z, this->player->yRot, this->player->xRot);

    // ticking network
    this->netData->tick();

    if (mpMode && client->isConnected()) { 
        PositionPacket* pos_packet = new PositionPacket(
            this->player->x, this->player->y, this->player->z,
            this->player->yRot, this->player->xRot
        );
        client->sendPacket(pos_packet);
    }
    ++this->ticks;
}
 
void CrossCraft::raycast() {
    if (this->hitResult != nullptr) {
        delete this->hitResult;
        this->hitResult = nullptr;
    }
    
    Ray ray = Ray::fromPlayer(this->player);
    
    this->hitResult = ray.trace(this->level, this->player, this->gamemode->getReachDistance());
}

void CrossCraft::render(float partialTicks) {
    // rotate player camera by mouse
    if (this->mouseGrabbed) {
        glViewport(0, 0, this->width, this->height);
        float xo = Mouse::getDX();
        float yo = Mouse::getDY();
        int iy = -1;
        if (this->settings->invertYMouse) iy = 1;
        this->player->turn(xo, yo * static_cast<float>(iy));
    }

    this->gamemode->applyCracks(partialTicks);
    // clear screen
    glClearColor(this->bgR, this->bgG, this->bgB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    this->fogDistance = (float)(512 >> (this->levelRenderer->drawDistance << 1));
    // setup fov projection
    this->setupCamera(partialTicks);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glEnable(GL_CULL_FACE);
    // cull chunks around player view & update
    Frustum& frustum = Frustum::getFrustum();
    this->levelRenderer->cull(frustum);
    this->levelRenderer->updateDirtyChunks(this->player);
    this->setupFog(0);
    // render level (layer = 0)
    glEnable(GL_FOG);
    this->levelRenderer->render(this->player, 0);
    Vec3D rvec = this->getPlayerVectorO(partialTicks);
    // Render Entities: (with normal3f lighting)
    this->setLighting(true);
    this->level->emesh->render(rvec, frustum, this->textures, partialTicks);
    // render multiplayer entities
    this->netData->render(partialTicks);
    this->setLighting(false);
    // render particles
    this->particleEngine->render(this->player, partialTicks, this->textures);
    // render surrounding ground
    this->levelRenderer->renderSurroundingGround();
    // render clouds
    glDisable(GL_LIGHTING);
    this->setupFog(-1);
    this->levelRenderer->renderClouds(partialTicks);
    this->setupFog(1);
    // render hits on tiles
    glEnable(GL_LIGHTING);
    if (this->hitResult != nullptr) {
        glCullFace(GL_BACK);
        glDisable(GL_LIGHTING);
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glEnable(GL_ALPHA_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 1.0f, 1.0f, ((float) std::sin(emscripten_get_now() / 100.0f) * 0.2f + 0.4f) * 0.5f);
        if (this->levelRenderer->cracks > 0.0f && dynamic_cast<SurvivalGameMode*>(this->gamemode)) {
            glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
            int texture = this->textures->loadTexture("terrain", GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, texture);
            glEnable(GL_TEXTURE_2D);
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
            glPushMatrix();
            int tileId = this->level->getTile(this->hitResult->x, this->hitResult->y, this->hitResult->z);
            Tile* tile = tileId > 0 ? Tile::tiles[tileId] : nullptr;
            Tessellator& t = Tessellator::getInstance();
            float offX = (tile->minX + tile->maxX) / 2.0f;
            float offY = (tile->minY + tile->maxY) / 2.0f;
            float offZ = (tile->minZ + tile->maxZ) / 2.0f;
            glTranslatef(((float)this->hitResult->x + offX), ((float)this->hitResult->y + offY), ((float)this->hitResult->z + offZ));
            glScalef(1.01f, 1.01f, 1.01f);
            glTranslatef(-((float)this->hitResult->x + offX), -((float)this->hitResult->y + offY), -((float)this->hitResult->z + offZ));
            t.begin();
            t._noColor();
            glDepthMask(false);
            if (tile == nullptr) {
                tile = Tile::tiles[Tile::rock->id];
            }

            for (int face = 0; face < 6; ++face) {
                tile->renderFace(t, this->hitResult->x, this->hitResult->y, this->hitResult->z, face, 240+(int)(this->levelRenderer->cracks*10.0f));
            }

            t.end();
            glDisable(GL_TEXTURE_2D);
            glDepthMask(true);
            glPopMatrix();

            if (this->mpMode && dynamic_cast<SurvivalGameMode*>(this->gamemode)) {

            }
        }
        glDisable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
        this->levelRenderer->renderHitOutline(this->hitResult, this->player, this->editMode, this->player->inventory->getCurrentBlock());
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_LIGHTING);
        glCullFace(GL_FRONT);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    this->setupFog(0);
    // Render Surrounfing Water
    this->levelRenderer->renderSurroundingWater();
    this->checkGlError("Render surrounding Water");
    // Render level water (layer = 1)
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_FOG);
    glDepthMask(true);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glColorMask(false, false, false, false);
    int waterChunks = this->levelRenderer->render(this->player, 1);
    glColorMask(true, true, true, true);
    if (waterChunks > 0) {
        this->levelRenderer->renderCollectedChunks();
    }
    this->checkGlError("Color Mask");
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    
    // render rain
    if (this->raining) {
        this->renderRain(partialTicks);
    }

    // render outline 
    if (this->hitResult != nullptr) {
        glDepthFunc(GL_LESS);
        glDisable(GL_ALPHA_TEST);
        // this->levelRenderer->renderHit(this->hitResult, this->player, this->editMode, this->player->inventory->getCurrentBlock());
        this->levelRenderer->renderHitOutline(this->hitResult, this->player, this->editMode, this->player->inventory->getCurrentBlock());
        glEnable(GL_ALPHA_TEST);
        glDepthFunc(GL_LEQUAL);
    }
    // reset mvp matrix
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_FOG);
    glClear(GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    // render hurt effect
    this->hurtEffect(partialTicks);
    if (this->settings->viewBobbing) {
        this->applyBobbing(partialTicks);
    }
    glEnable(GL_TEXTURE_2D);
    this->renderHeldBlock(partialTicks);
    glDisable(GL_TEXTURE_2D);
    // render gui
    this->drawGui(partialTicks);
    glfwSwapBuffers(window);
}

void CrossCraft::drawGui(float partialTicks) {
    int screenWidth = this->width * 240 / this->height;
    int screenHeight = this->height * 240 / this->height;
    int xMouse = Mouse::getX() * screenWidth / this->width;
    int yMouse = Mouse::getY() * screenHeight / this->height;
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, screenWidth, screenHeight, 0.0f, 100.0f, 300.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -200.0f);
    Tessellator& t = Tessellator::getInstance();
    this->checkGlError("GUI: Init");
    // render hud
    glPushMatrix();
    if (this->hud != nullptr) {
        this->hud->render(this->player, this->level, partialTicks);
    }
    glPopMatrix();
    this->checkGlError("GUI: Draw selected");
    this->font->drawShadow(VERSION_STRING, 2, 2, 0xFFFFFF);
    if (this->settings->showFPS) {
        // Оно роняет фпс сильно
        // size_t maxMemory = emscripten_get_heap_max();
        // size_t totalMemory = emscripten_get_heap_size();

        // struct mallinfo info = mallinfo(); 
        // size_t usedMemory = info.uordblks;
        // size_t freeMemory = totalMemory - usedMemory;

        // size_t showFM = maxMemory - freeMemory;

        // size_t maxMB = maxMemory / (1024 * 1024);
        // size_t totalMB = totalMemory / (1024 * 1024);

        // if (maxMemory == 0) maxMemory = 1;

        // size_t freePercent = ((uint64_t)(showFM) * 100) / maxMemory;
        // std::string strFree = "Free memory: " + std::to_string(freePercent) + "% of " + std::to_string(maxMB) + "MB";

        // size_t allocPercent = ((uint64_t)(totalMemory) * 100) / maxMemory;
        // std::string strAlloc = "Allocated memory: " + std::to_string(allocPercent) + "% (" + std::to_string(totalMB) + "MB)";

        this->font->drawShadow(this->fpsString, 2, 12, 0xFFFFFF);
        // this->font->drawShadow(strFree.c_str(), 2, 22, 0xFFFFFF);
        // this->font->drawShadow(strAlloc.c_str(), 2, 32, 0xFFFFFF);
    }
    this->checkGlError("GUI: Draw text");
    int wc = screenWidth / 2;
    int hc = screenHeight / 2;
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    // TODO: сделать рендер в виде одного прямоугольника с текстурой, должно уменьшить затраты на рендер
    t.begin();
    t.vertex((float)(wc + 1), (float)(hc - 4), 0.0F);
    t.vertex((float)(wc - 0), (float)(hc - 4), 0.0F);
    t.vertex((float)(wc - 0), (float)(hc + 5), 0.0F);
    t.vertex((float)(wc + 1), (float)(hc + 5), 0.0F);
    t.vertex((float)(wc + 5), (float)(hc - 0), 0.0F);
    t.vertex((float)(wc - 4), (float)(hc - 0), 0.0F);
    t.vertex((float)(wc - 4), (float)(hc + 1), 0.0F);
    t.vertex((float)(wc + 5), (float)(hc + 1), 0.0F);
    t.end();
    this->checkGlError("GUI: Draw crosshair");
    this->chatGui->render(this->font, screenWidth, screenHeight);
    // render player list (Creative & multiplayer)
    if (this->mpMode && Keyboard::isKeyDown(GLFW_KEY_TAB)) {
        playerListScreen->init(this, this->width * 240 / this->height, this->height * 240 / this->height);
        playerListScreen->render(xMouse, yMouse);
    }
    // render current screen
    if (this->screen != nullptr) {
        this->screen->render(xMouse, yMouse);
    }
}

void CrossCraft::renderRain(float partialTicks) {
    glBindTexture(GL_TEXTURE_2D, this->textures->loadTexture("/rain.png", GL_NEAREST));
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glNormal3f(0.0f, 1.0f, 0.0f);
    int x = (int)this->player->x;
    int y = (int)this->player->y;
    int z = (int)this->player->z;
    Tessellator& t = Tessellator::getInstance();
    for (int px = x - 5; px <= x + 5; ++px) {
        for (int pz = z - 5; pz <= z + 5; ++pz) {
            int py = this->level->getHighestTile(px, pz);
            int miny = y - 5;
            int maxy = y + 5;
            if (miny < py) {
                miny = py;
            }
            if (maxy < py) {
                maxy = py;
            }

            if (miny != maxy) {
                float animOffset = ((float)((this->ticks + px * 3121 + pz * 418711) % 32) + partialTicks) / 32.0f;

                float dx = (float)px + 0.5f - x;
                float dz = (float)pz + 0.5f - z;
                float dist = std::sqrt(dx*dx + dz*dz) / 5.0f;
                float alpha = (1.0f - dist*dist) * 0.7f;

                glColor4f(1.0f, 1.0f, 1.0f, alpha);

                float v0 = (float)miny * 2.0f / 8.0f + animOffset * 2.0f;
                float v1 = (float)maxy * 2.0f / 8.0f + animOffset * 2.0f;

                float x0 = (float)px;
                float x1 = (float)(px+1);
                float y0 = (float)miny;
                float y1 = (float)maxy;
                float z0 = (float)pz;
                float z1 = (float)(pz+1);

                t.begin();
                t.vertexUV(x0, y0, z0, 0.0f, v0);
                t.vertexUV(x1, y0, z1, 2.0f, v0);
                t.vertexUV(x1, y1, z1, 2.0f, v1);
                t.vertexUV(x0, y1, z0, 0.0f, v1);

                t.vertexUV(x0, y0, z1, 0.0f, v0);
                t.vertexUV(x1, y0, z0, 2.0f, v0);
                t.vertexUV(x1, y1, z0, 2.0f, v1);
                t.vertexUV(x0, y1, z1, 0.0f, v1);
                t.end();
            }
        }
    }

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void CrossCraft::fill(int x0, int y0, int x1, int y1, int col) {
    float a = (col >> 24 & 255) / 255.0f;
    float r = (col >> 16 & 255) / 255.0f;
    float g = (col >> 8 & 255) / 255.0f;
    float b = (col & 255) / 255.0f;
    Tessellator& t = Tessellator::getInstance();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    t.begin();
    t.vertex((float)x0, (float)y1, 0.0f);
    t.vertex((float)x1, (float)y1, 0.0f);
    t.vertex((float)x1, (float)y0, 0.0f);
    t.vertex((float)x0, (float)y0, 0.0f);
    t.end();
    glDisable(GL_BLEND);
}

void CrossCraft::setupCamera(float partialTicks) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float fov = 70.0f;
    if (this->player->health <= 0) {
        float zoom = this->player->deathTime + partialTicks;
        fov /= (1.0f - 500.0f / (zoom + 500.0f)) * 2.0f + 1.0f;
    }
    float aspectRatio = static_cast<float>(this->width) / static_cast<float>(this->height);
    gluPerspective(fov, aspectRatio, 0.05, this->fogDistance);
    this->hurtEffect(partialTicks);
    if (this->settings->viewBobbing) {
        this->applyBobbing(partialTicks);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    this->moveCameraToPlayer(partialTicks);
}

void CrossCraft::moveCameraToPlayer(float partialTicks) {
    glTranslatef(0.0f, 0.0f, -0.3f);
    glRotatef(this->player->xRot, 1.0f, 0.0f, 0.0f);
    glRotatef(this->player->yRot, 0.0f, 1.0f, 0.0f);
    float x = this->player->xo + (this->player->x - this->player->xo) * partialTicks;
    float y = this->player->yo + (this->player->y - this->player->yo) * partialTicks;
    float z = this->player->zo + (this->player->z - this->player->zo) * partialTicks;
    glTranslatef(-x, -y, -z);
}

void CrossCraft::checkGlError(const char str[]) {
    return;
    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR) {
        const GLubyte* errorString = gluErrorString(errorCode);
        printf("########## GL ERROR ##########\n");
        printf("%s\n", str);
        printf("@ %s\n", errorString);
        printf("%i: %s\n", errorCode, errorString);
    }
} 

void CrossCraft::setupFog(int layer) {
    if (layer == -1) {
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 0.0f);
        glFogf(GL_FOG_END, this->fogDistance);
        glFogfv(GL_FOG_COLOR, getBuffer(0.92f, 0.98f, 1.0f, 1.0f));
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, getBuffer(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        Tile* currentTile = Tile::tiles[this->level->getTile((int)this->player->x, (int)(this->player->y+0.12f), (int)this->player->z)];
        if (currentTile != nullptr && currentTile->getLiquidType() == LiquidType::WATER) {
            glFogi(GL_FOG_MODE, GL_EXP);
            glFogf(GL_FOG_DENSITY, 0.1f);
            glFogfv(GL_FOG_COLOR, getBuffer(0.02f, 0.02f, 0.2f, 1.0f));
            this->bgR = 0.02f;
            this->bgG = 0.02f;
            this->bgB = 0.2f;
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, getBuffer(0.4f, 0.4f, 0.9f, 1.0f));
        } else if (currentTile != nullptr && currentTile->getLiquidType() == LiquidType::LAVA) {
            glFogi(GL_FOG_MODE, GL_EXP);
            glFogf(GL_FOG_DENSITY, 2.0f);
            glFogfv(GL_FOG_COLOR, getBuffer(0.6f, 0.1f, 0.0f, 1.0f));
            this->bgR = 0.6f;
            this->bgG = 0.1f;
            this->bgB = 0.0f;
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, getBuffer(0.4f, 0.3f, 0.3f, 1.0f));
        } else if (layer == 0) {
            this->bgR = 0.92f;
            this->bgG = 0.98f;
            this->bgB = 1.0f;
            glFogi(GL_FOG_MODE, GL_LINEAR);
            glFogf(GL_FOG_START, 0.0f);
            glFogf(GL_FOG_END, this->fogDistance);
            glFogfv(GL_FOG_COLOR, getBuffer(0.92f, 0.98f, 1.0f, 1.0f));
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, getBuffer(1.0f, 1.0f, 1.0f, 1.0f));
        } else if (layer == 1) {
            this->bgR = 0.92f;
            this->bgG = 0.98f;
            this->bgB = 1.0f;
            glFogi(GL_FOG_MODE, GL_EXP);
            glFogf(GL_FOG_DENSITY, 0.01f);
            glFogfv(GL_FOG_COLOR, this->fogColor1.data());
            float br = 1.0f;
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, getBuffer(br, br, br, 1.0f));
        }

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT, GL_AMBIENT);
        glEnable(GL_LIGHTING);
    }
}

void CrossCraft::applyBobbing(float partialTicks) {
    float dist = this->player->walkDist - this->player->walkDistO;
    dist = this->player->walkDist + dist * partialTicks;
    float bob = this->player->oBob + (this->player->bob - this->player->oBob) * partialTicks;
    float tilt = this->player->oTilt + (this->player->tilt - this->player->oTilt) * partialTicks;
    glTranslatef(std::sin(dist * M_PI) * bob * 0.5f, -std::abs(std::cos(dist * M_PI) * bob), 0.0f);
    glRotatef(std::sin(dist * M_PI) * bob * 3.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(std::abs(std::cos(dist * M_PI + 0.2f) * bob) * 5.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(tilt, 1.0f, 0.0f, 0.0f);
}

void CrossCraft::renderHeldBlock(float partialTicks) {
    HeldBlock* held = this->heldBlock;
    float iPos = held->lastPos + (held->pos - held->lastPos) * partialTicks;
    Player* player = this->player;
    glPushMatrix();
    glRotatef(player->xRotO + (player->xRot - player->xRotO) * partialTicks, 1.0f, 0.0f, 0.0f);
    glRotatef(player->yRotO + (player->yRot - player->yRotO) * partialTicks, 0.0f, 1.0f, 0.0f);
    this->setLighting(true);
    glPopMatrix();
    glPushMatrix();
    float scale = 0.8f;
    if (held->moving) {
        float walkCycle = ((float)held->offset + partialTicks) / 7.0f;
        float sinWalk = std::sin(walkCycle * M_PI);
        glTranslatef(-std::sin(std::sqrt(walkCycle) * M_PI) * 0.4f, std::sin(std::sqrt(walkCycle) * M_PI * 2.0f) * 0.2f, -sinWalk * 0.2f);

    }

    glTranslatef(0.7f * scale, -0.65f * scale - (1.0f - iPos) * 0.6f, -0.9f * scale);
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    glEnable(GL_NORMALIZE);
    if (held->moving) {
        float walkCycle = (held->offset + partialTicks) / 7.0f;
        float motionAngle = std::sin(walkCycle * walkCycle * M_PI);
        glRotatef(std::sin(std::sqrt(walkCycle) * M_PI) * 80.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(-motionAngle * 20.0f, 1.0f, 0.0f, 0.0f);
    }

    float brightness = held->cc->level->getBrightness((int)player->x, (int)player->y, (int)player->z);
    // float brightness = 1.0f;
    glColor4f(brightness, brightness, brightness, 1.0f);
    Tessellator& t = Tessellator::getInstance();

    if (held->tile != nullptr) {
        float tileScale = 0.4f;
        glScalef(tileScale, tileScale, tileScale);
        glTranslatef(-0.5f, -0.5f, -0.5f);
        glBindTexture(GL_TEXTURE_2D, this->textures->loadTexture("terrain", GL_NEAREST));
        held->tile->renderPreview(t);
    } else {
        player->bindTexture(this);
        glScalef(1.0f, -1.0f, -1.0f);
        glTranslatef(0.0f, 0.2f, 0.0f);
        glRotatef(-120.0f, 0.0f, 0.0f, 1.0f);
        glScalef(1.0f, 1.0f, 1.0f);

        float modelScale = 0.0625f;
        ModelPart* leftArm = player->getModel()->arm0;
        if (!leftArm->compiled) {
            leftArm->compileDisplayList(modelScale);
        }
        glCallList(leftArm->displayList);
    }

    glDisable(GL_NORMALIZE);
    glPopMatrix();

    this->setLighting(false);
}

void CrossCraft::hurtEffect(float partialTicks) {
    float hurtTime = this->player->hurtTime - partialTicks;
    if (this->player->health <= 0) {
        float deathProgress = partialTicks + this->player->deathTime;
        glRotatef(40.0f - 8000.0f / (deathProgress + 200.0f), 0.0f, 0.0f, 1.0f);
    }

    if (hurtTime >= 0.0f) {
        float progress = hurtTime / this->player->hurtDuration;
        float shakeAngle = std::sin(progress * progress * progress * progress * M_PI);
        float direction = this->player->hurtDir;
        glRotatef(-direction, 0.0f, 1.0f, 0.0f);
        glRotatef(-shakeAngle * 14.0f, 0.0f, 0.0f, 1.0f);
        glRotatef(direction, 0.0f, 1.0f, 0.0f);
    }
}

void CrossCraft::setLighting(bool enable) {
    if (!enable) {
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
    } else {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        Vec3D vec = Vec3D(0.0f, -1.0f, 0.5f).normalize();
        glLightfv(GL_LIGHT0, GL_POSITION, this->getBuffer(vec.x, vec.y, vec.z, 0.0f));
        glLightfv(GL_LIGHT0, GL_DIFFUSE, this->getBuffer(0.3f, 0.3f, 0.3f, 1.0f));
        glLightfv(GL_LIGHT0, GL_AMBIENT, this->getBuffer(0.0f, 0.0f, 0.0f, 1.0f));
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, this->getBuffer(0.7f, 0.7f, 0.7f, 1.0f));
    }
}

void CrossCraft::beginLevelLoading(const char title[]) {
    std::string t = std::string(title);
    this->progressbar->startProgressBar(t);
}

void CrossCraft::levelLoadUpdate(const char* status) {
    std::string s = std::string(status);
    this->progressbar->updateProgressStatus(s);
}

void CrossCraft::levelLoadProgress(int progress) {
    this->progressbar->updateProgressState(progress);
}

bool CrossCraft::loadLevel(const char username[], int levelid) {
    bool loaded = this->levelIO->loadOnline(this->level, this->serverHost, username, levelid);

    if (!loaded) {
        this->level->player = this->player;
        return false;
    } else {
        // delete this->player;
        this->player = (Player*)this->level->player;
        this->player->resetPos();
        this->gamemode->prepareLevel(this->level);
        this->gamemode->apply(this->level);
        this->gamemode->apply(this->player);
        // this->level->addEntity(this->player);

        return true;
    }
}

void CrossCraft::saveLevel(int levelId, const char levelname[]) {
    this->levelIO->saveOnline(this->level, this->serverHost, this->userData->username, this->userData->sessionid, levelname, levelId);
}

void CrossCraft::generateNewLevel(int width, int height, int depth) {
    const char* username = (this->userData != nullptr) ? this->userData->username.c_str() : "noname";
    this->levelGen->generateLevel(this->level, username, width, height, depth);
    this->level->player = nullptr;
    // delete this->player;
    this->player = new Player(this->level, this->settings);
    this->level->player = this->player;
    Logger::logf(PREFIX_DEBUG, "Before player reset\n");

    this->player->resetPos();
    this->gamemode->preparePlayer(this->player);
    this->gamemode->apply(this->level);
    this->gamemode->prepareLevel(this->level);
    this->gamemode->apply(this->player);
    // if (this->level != nullptr) {
    //     this->level->player = this->player;
    //     this->level->addEntity(this->player);
    // }
    GC_gcollect(); // да, сборщик мусора в c++, и чё?
}

Vec3D* CrossCraft::getPlayerVector(float partialTicks) {
    float x = this->player->xo + (this->player->x - this->player->xo) * partialTicks;
    float y = this->player->yo + (this->player->y - this->player->yo) * partialTicks;
    float z = this->player->zo + (this->player->z - this->player->zo) * partialTicks;
    return new Vec3D(x, y, z);
}

Vec3D CrossCraft::getPlayerVectorO(float partialTicks) {
    float x = this->player->xo + (this->player->x - this->player->xo) * partialTicks;
    float y = this->player->yo + (this->player->y - this->player->yo) * partialTicks;
    float z = this->player->zo + (this->player->z - this->player->zo) * partialTicks;
    return Vec3D(x, y, z);
}

void CrossCraft::showErrorScreen(const std::string& title, const std::string& reason) {
    this->inErrorState = true;
    this->errorTitle = title;
    this->errorReason = reason;
}

void CrossCraft::drawErrorScreen() {
    int screenWidth = this->width * 240 / this->height;
    int screenHeight = this->height * 240 / this->height;

    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, screenWidth, screenHeight, 0.0f, 100.0f, 300.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -200.0f);

    glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    
    glColor4f(0.42f, 0.08f, 0.07f, 1.0f);  // #6b1412
    glVertex2f(screenWidth, 0);
    glVertex2f(0, 0);
    
    glColor4f(0.24f, 0.02f, 0.02f, 1.0f); // #3d0505
    glVertex2f(0, screenHeight);
    glVertex2f(screenWidth, screenHeight);
    
    glEnd();

    glPushMatrix();
    glTranslatef(screenWidth / 2.0f, screenHeight / 2.0f - 20.0f, 0.0f);
    glScalef(2.0f, 2.0f, 2.0f);
    this->font->drawCentered(this->errorTitle, 0, 0, 0xFFFFFFFF);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(screenWidth / 2.0f, screenHeight / 2.0f + 10.0f, 0.0f);
    this->font->drawCentered(this->errorReason, 0, 0, 0xFFFFFFFF);
    glPopMatrix();
}

void CrossCraft::connectToServer() {
    if (currentUrlIndex >= connectionUrls.size()) {
        this->showErrorScreen("Connection Failed", "All connection methods failed.");
        return;
    }

    std::string urlToTry = connectionUrls[currentUrlIndex];
    currentUrlIndex++;

    std::string status = "Connecting... (Attempt " + std::to_string(currentUrlIndex) + "/" + std::to_string(connectionUrls.size()) + ")";
    this->beginLevelLoading("Connecting to server...");
    this->levelLoadUpdate(status.c_str()); 
    this->levelLoadProgress(10);

    if (client == nullptr) {
        client = new Client();
        this->netData->handler->client = client;
    }
    
    // client->setOnConnect([this]() {
    //     this->canRender = false;
    //     Logger::logf(PREFIX_NETWORK, "WebSocket connection opened, waiting for Server ID...\n");
    // });
    
    client->setOnPacket([this](Packet* packet) {
        this->netData->handler->handleNetworkPacket(packet);
    });

    client->setOnError([this](std::string error) {
        Logger::logf(PREFIX_NETWORK, "Attempt %d failed: %s\n", this->currentUrlIndex, error.c_str());
        this->connectToServer(); 
    });

    client->setOnClose([this](std::string reason) {
        this->showErrorScreen("Disconnected", reason);
    });
    
    this->beginLevelLoading("Connecting to server...");
    this->levelLoadUpdate("Establishing connection"); 
    this->levelLoadProgress(10);
    client->connect(urlToTry);
}

void CrossCraft::connectError(std::string error) {
    Logger::logf(PREFIX_NETWORK, "Connection error!\n");
    this->showErrorScreen("Connection Error!", error);
}

void CrossCraft::handleNetworkPacket(Packet* packet) {
    
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void showCrashScreen(const char* msg) {
        if (CrossCraft::instance) {
            printf("Creating crash screen for: %s\n", msg);
            emscripten_cancel_main_loop(); 
            CrossCraft::instance->showErrorScreen("Runtime Error", msg);
            glfwMakeContextCurrent(CrossCraft::instance->window);
            CrossCraft::instance->drawErrorScreen();
            glfwSwapBuffers(CrossCraft::instance->window);
            glFlush();
            glFinish();
            printf("Crash screen rendered.\n");
        }
    }
}