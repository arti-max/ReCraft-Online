#pragma once
#include "gui/Screen.hpp"
#include "render/Textures.hpp"
#include "render/Tessellator.hpp"
#include <vector>


class BlockSelectScreen : public Screen {
protected:
    void mouseClicked(int x, int y, int button) override;
public:
    BlockSelectScreen();
    virtual ~BlockSelectScreen() = default;

    void init() override;
    void render(int xMouse, int yMouse) override;
private:
    int blocksPerRow = 8;
    std::vector<int> blocks;
    int getBlockAt(int x, int y);
};