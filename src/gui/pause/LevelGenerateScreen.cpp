#include "gui/pause/LevelGenerateScreen.hpp"
#include "CrossCraft.hpp"

LevelGenerateScreen::LevelGenerateScreen(Screen* parent) : Screen::Screen() {
    this->parent = parent;
}

void LevelGenerateScreen::init() {
    this->buttons.clear();
    this->buttonNames.push_back("Small");
    this->buttonNames.push_back("Normal");
    this->buttonNames.push_back("Huge");

    for (int i = 0; i < 3; ++i) {
        this->buttons.push_back(new Button(i, this->width / 2 - 100, this->height / 4 + i * 24, 200, 20, this->buttonNames[i]));
    }

    this->buttons.push_back(new Button(5, this->width / 2 - 100, this->height / 4 + 144, 200, 20, "Cancel"));
}

void LevelGenerateScreen::buttonClicked(Button* btn) {
    if (btn->enabled) {
        int w = 256;
        int h = 256;
        int d = 64;

        if (btn->id == 0) {
            w = 128;
            h = 128;
        }
        if (btn->id == 1) {
            w = 256;
            h = 256;
        }
        if (btn->id == 2) {
            w = 512;
            h = 512;
        }
        if (btn->id == 5) {
            this->cc->setScreen(parent);
            return;
        }

        this->cc->generateNewLevel(w, h, d);
        this->cc->setScreen(nullptr);
        this->cc->waitingForFocus = true;
    }
}

void LevelGenerateScreen::render(int xMouse, int yMouse) {
    this->fillGradient(0, 0, this->width, this->height, 537199872, -1607454624);
    this->drawCenteredString(this->title.c_str(), this->width / 2, 40, 0xFFFFFFFF);
    Screen::render(xMouse, yMouse);
}