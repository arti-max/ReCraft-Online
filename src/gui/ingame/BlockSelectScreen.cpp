#include "CrossCraft.hpp"
#include "level/tile/Tile.hpp"
#include "util/Logger.hpp"
#include "gui/ingame/BlockSelectScreen.hpp"

BlockSelectScreen::BlockSelectScreen() {}

void BlockSelectScreen::init() {
    this->grabMouse = true;
    this->blocks = {
        Tile::rock->id, Tile::cobblestone->id, Tile::dirt->id, Tile::wood->id, Tile::log->id, Tile::leaves->id, Tile::bush->id, Tile::yellowFlower->id,
        Tile::redFlower->id, Tile::brownMushroom->id, Tile::redMushroom->id, Tile::sand->id, Tile::gravel->id, Tile::glass->id, Tile::sponge->id, Tile::goldBlock->id,
        Tile::wool1->id, Tile::wool2->id, Tile::wool3->id, Tile::wool4->id, Tile::wool5->id, Tile::wool6->id, Tile::wool7->id, Tile::wool8->id,
        Tile::wool9->id, Tile::wool10->id, Tile::wool11->id, Tile::wool12->id, Tile::wool13->id, Tile::wool14->id, Tile::wool15->id, Tile::wool16->id,
    };
}

void BlockSelectScreen::render(int xMouse, int yMouse) {
    glDisable(GL_DEPTH_TEST); 
    this->fillGradient(this->width/2-120, 30, this->width/2+120, 180, -1878719232, -1070583712);
    this->drawCenteredString("Select block", this->width/2, 40, 0xFFFFFFFF);
    int startX = this->width/2-24*4;
    int startY = this->height/2-24*2;
    Tessellator& t = Tessellator::getInstance();

    int index = -1;
    int relativeX = xMouse - startX;
    int relativeY = yMouse - startY;
    int col = relativeX / 24;
    int row = relativeY / 24;
    index = row * 8 + col;
    if (relativeX < 0 || relativeY < 0) index=-1;
    if (col >= 8) index=-1;
    if (index >= this->blocks.size()) index=-1;

    if (index >= 0) {
        int tileX = startX + (index % 8) * 24;
        int tileY = startY + (index / 8) * 24;
        this->fillGradient(tileX, tileY, tileX+24, tileY+24, -1862270977, -1056964609);
    }

    glBindTexture(GL_TEXTURE_2D, this->cc->textures->loadTexture("terrain", GL_NEAREST));
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT); 

    glEnable(GL_RESCALE_NORMAL);
    glEnable(GL_COLOR_MATERIAL);


    for (int i=0; i<this->blocks.size(); i++) {
        int tileX = startX + (i % 8) * 24;
        int tileY = startY + (i / 8) * 24;
        glPushMatrix();
        glTranslatef(tileX+12.0f, tileY+12.0f, -50.0f);
        if (i == index) {
            glScalef(12.0f, 12.0f, 12.0f);
        } else {
            glScalef(10.0f, 10.0f, 10.0f);
        }
        glRotatef(-30.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(45.0, 0.0f, 1.0f, 0.0f);
        glTranslatef(-1.5f, 0.5f, 0.5f);
        glScalef(-1.0f, -1.0f, -1.0f);
        
        t.begin();
        Tile::tiles[this->blocks[i]]->render(t, this->cc->level, -2, 0, 0);
        t.end();
        
        glPopMatrix();
    }

    glDisable(GL_RESCALE_NORMAL);
    glDisable(GL_COLOR_MATERIAL);
}

void BlockSelectScreen::mouseClicked(int x, int y, int button) {
    if (button == 0) {
        int block = this->getBlockAt(x, y);
        if (block != -1) {
            int inOtherSlot = this->cc->player->inventory->inInventory(block, this->cc->player->inventory->selectedSlot);
            if (inOtherSlot != -1) {
                int otherSlotBlock = this->cc->player->inventory->getInSlot(inOtherSlot);
                int prevBlock = this->cc->player->inventory->getCurrentBlock();
                this->cc->player->inventory->addBlockToSlot(inOtherSlot, prevBlock);
            }
            this->cc->player->inventory->addBlockToSlot(this->cc->player->inventory->selectedSlot, block);
        }
        this->cc->setScreen((Screen*)nullptr);
        this->cc->waitingForFocus = true;
    }
}

int BlockSelectScreen::getBlockAt(int x, int y) {
    int startX = this->width/2-24*4;
    int startY = this->height/2-24*2;
    int relativeX = x - startX;
    int relativeY = y - startY;
    if (relativeX < 0 || relativeY < 0) return -1;
    int col = relativeX / 24;
    int row = relativeY / 24;
    if (col >= 8) return -1;
    int index = row * 8 + col;
    if (index >= this->blocks.size()) return -1;
    return this->blocks[index];
}