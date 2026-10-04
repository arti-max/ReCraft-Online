#include "gui/ingame/Hud.hpp"
#include <GL/gl.h>
#include "CrossCraft.hpp"
#include "Logger.hpp"
#include <string>

Hud::Hud(CrossCraft* cc, Textures* textures, int width, int height) : 
    cc(cc), textures(textures), width(width), height(height) {}

void Hud::drawTexturedModalRect(int x, int y, int u, int v, int w, int h) {
    float uScale = 1.0f / 256.0f;
    float vScale = 1.0f / 256.0f;
    Tessellator& t = Tessellator::getInstance();
    
    t.begin();
    t.vertexUV(x,     y + h, 0.0f, u * uScale,       (v + h) * vScale);
    t.vertexUV(x + w, y + h, 0.0f, (u + w) * uScale, (v + h) * vScale);
    t.vertexUV(x + w, y,     0.0f, (u + w) * uScale, v * vScale);
    t.vertexUV(x,     y,     0.0f, u * uScale,       v * vScale);
    t.end();
}

void Hud::render(Player* player, Level* level, float partialTicks) {
    int screenWidth = this->width;
    int screenHeight = this->height;
    int xStart = (screenWidth - 182) / 2;
    int yStart = screenHeight - 22; 

    glDisable(GL_DEPTH_TEST); 

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, textures->loadTexture("/gui/gui.png", GL_NEAREST));
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawTexturedModalRect(xStart, yStart, 0, 0, 182, 22);

    int slot = player->inventory->selectedSlot; 
    drawTexturedModalRect(xStart - 1 + slot * 20, yStart - 1, 0, 22, 24, 24);
    
    glBindTexture(GL_TEXTURE_2D, textures->loadTexture("/gui/icons.png", GL_NEAREST));
    this->drawTexturedModalRect(screenWidth / 2 - 7, screenHeight /2 - 7, 0, 0, 16, 16);
    bool blink = this->cc->player->invulnerableTime / 3 % 2 == 1;
    if (this->cc->player->invulnerableTime < 10) {
        blink = false;
    }

    int health = this->cc->player->health;
    int lasth = this->cc->player->lastHealth;
    this->random->setSeed(this->ticks * 312871);

    if (this->cc->gamemode->isSurvival()) {
        for (int i = 0; i < 10; ++i) {
            int heartYOffset = 0;
            if (blink) {
                heartYOffset = 1;
            }

            int heartX = this->width / 2 - 91 + (i*8);
            int heartY = this->height - 32;
            if (health <= 4) {
                heartY += this->random->nextInt(2);
            }

            this->drawTexturedModalRect(heartX, heartY, 16 + heartYOffset * 9, 0, 9, 9);

            if (blink) {
                if (i * 2 + 1 < lasth) {
                    this->drawTexturedModalRect(heartX, heartY, 70, 0, 9, 9);
                }
                if (i * 2 + 1 == lasth) {
                    this->drawTexturedModalRect(heartX, heartY, 79, 0, 9, 9);
                }
            }

            if (i * 2 + 1 < health) {
                this->drawTexturedModalRect(heartX, heartY, 52, 0, 9, 9);
            }
            if (i * 2 + 1 == health) {
                this->drawTexturedModalRect(heartX, heartY, 61, 0, 9, 9);
            }
        } 

        if (this->cc->player->isUnderWater()) {
            int airFull = (int)std::ceil((this->cc->player->airSupply - 2) * 10.0f / 300.0f);
            int airTotal = (int)std::ceil(this->cc->player->airSupply * 10.0f / 300.0f);
            int airEmpty = airTotal - airFull;

            for (int i = 0; i < airFull + airEmpty; ++i) {
                int bubbleX = this->width / 2 - 91 + (i*8);
                int bubbleY = this->height - 32 - 9;
                if (i < airFull) {
                    this->drawTexturedModalRect(bubbleX, bubbleY, 16, 18, 9, 9);
                } else {
                    this->drawTexturedModalRect(bubbleX, bubbleY, 25, 18, 9, 9);
                }
            }
        }

        std::string arrows = format("Arrows: %i", player->inventory->getArrowCount());
        this->cc->font->drawShadow(arrows, this->width / 2 + 24, this->height - 32, 0xFFFFFFFF);

        std::string score = format("Score: &e%i", player->score);
        this->cc->font->drawShadow(score, this->width - this->cc->font->width(score)-8, 2, 0xFFFFFFFF);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT); 

    Tessellator& t = Tessellator::getInstance();

    for (int i = 0; i < 9; ++i) {
        int tileId = player->inventory->slots[i];
        int count = player->inventory->count[i];
        int popTime = player->inventory->popTime[i];
        if (tileId > 0 && Tile::tiles[tileId] != nullptr) {
            float itemX = this->width / 2 - 90 + i * 20;
            float itemY = this->height - 16;

            glPushMatrix();
            glTranslatef(itemX, itemY, -50.0f);

            if (popTime > 0) {
                float popFraction = (popTime - partialTicks) / 5.0f;
                float popOffsetY = -std::sin(popFraction * popFraction * M_PI) * 8.0f;
                float popScaleX = std::sin(popFraction * popFraction * M_PI) + 1.0f;
                float popScaleY = std::sin(popFraction * M_PI) + 1.0f;
                glTranslatef(10.0f, popOffsetY + 10.0f, 0.0f);
                glScalef(popScaleX, popScaleY, 1.0f);
                glTranslatef(-10.0f, -10.0f, 0.0f);
            }

            glEnable(GL_RESCALE_NORMAL);
            glEnable(GL_COLOR_MATERIAL);
            glEnable(GL_TEXTURE_2D);
            
            glScalef(10.0f, 10.0f, 10.0f);
            glTranslatef(1.0f, 0.5f, 0.0f);
            glRotatef(-30.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(45.0, 0.0f, 1.0f, 0.0f);
            glTranslatef(-1.5f, 0.5f, 0.5f);
            glScalef(-1.0f, -1.0f, -1.0f);
            
            glBindTexture(GL_TEXTURE_2D, textures->loadTexture("terrain", GL_NEAREST));

            t.begin();
            Tile::tiles[tileId]->render(t, level, -2, 0, 0);
            t.end();

            glDisable(GL_RESCALE_NORMAL);
            glDisable(GL_COLOR_MATERIAL);
            
            glPopMatrix();
            glPushMatrix();
            if (count > 1) {
                std::string txt = std::to_string(count);
                this->cc->font->drawShadow(txt, itemX+19 - this->cc->font->width(txt), itemY+6, 0xFFFFFFFF);
            }
            glPopMatrix();
        }
    }
    
    
}
