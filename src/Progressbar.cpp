#include "Progressbar.hpp"
#include "render/Textures.hpp"
#include "render/Tessellator.hpp"
#include "gui/Font.hpp"
#include <GL/gl.h>
#include "CrossCraft.hpp"

Progressbar::Progressbar(int width, int height, Textures* textures) {
    this->width = width;
    this->height = height;
    this->textures = textures;
    this->font = new Font("default", this->textures);
}

void Progressbar::startProgressBar(std::string& title) {
    this->title = title;
    this->status = "";
    this->lastProgress = -1;
    if (this->height <= 0) {
        this->height = (CrossCraft::instance->height > 0) ? CrossCraft::instance->height : 480;
    }
    if (this->width <= 0) {
        this->width = (CrossCraft::instance->width > 0) ? CrossCraft::instance->width : 640;
    }
    int screenWidth = this->width * 240 / this->height;
    int screenHeight = this->height * 240 / this->height;
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, (double)screenWidth, (double)screenHeight, 0.0f, 100.0f, 300.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -200.0f);
}

void Progressbar::updateProgressStatus(std::string& status) {
    this->status = status;
}

void Progressbar::updateProgressState(int progress) {
    if (progress == this->lastProgress) {
        return;
    }
    if (this->height <= 0) {
        this->height = (CrossCraft::instance->height > 0) ? CrossCraft::instance->height : 480;
    }
    if (this->width <= 0) {
        this->width = (CrossCraft::instance->width > 0) ? CrossCraft::instance->width : 640;
    }

    if (std::abs(progress - this->lastProgress) < 2 && progress != 100 && progress != 0) {
        return;
    }
    this->lastProgress = progress;
    int screenWidth = this->width * 240 / this->height;
    int screenHeight = this->height * 240 / this->height;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Tessellator& t = Tessellator::getInstance();
    glEnable(GL_TEXTURE_2D);
    int id = this->textures->loadTexture("dirt", GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, id);
    t.begin();
    t.color(64.0f / 255.0f, 64.0f / 255.0f, 64.0f / 255.0f);
    float s = 32.0f;
    t.vertexUV(0.0f, (float)screenHeight, 0.0f, 0.0f, (float)screenHeight / s);
    t.vertexUV((float)screenWidth, (float)screenHeight, 0.0f, (float)screenWidth / s, (float)screenHeight / s);
    t.vertexUV((float)screenWidth, 0.0f, 0.0f, (float)screenWidth / s, 0.0f);
    t.vertexUV(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    t.end();
    
    if (progress >= 0) {
        glDisable(GL_TEXTURE_2D);
        int barX = screenWidth / 2 - 50;
        int barY = screenHeight / 2 + 16;
        int barWidth = 100;
        int barHeight = 2;

        t.begin();
        t.color(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f);
        t.vertex((float)barX, (float)barY, 0.0f);
        t.vertex((float)barX, (float)(barY + barHeight), 0.0f);
        t.vertex((float)(barX + barWidth), (float)(barY + barHeight), 0.0f);
        t.vertex((float)(barX + barWidth), (float)barY, 0.0f);
        t.end();

        t.begin();
        t.color(128.0f / 255.0f, 255.0f / 255.0f, 128.0f / 255.0f);
        // glColor3f(128.0f / 255.0f, 255.0f / 255.0f, 128.0f / 255.0f);
        t.vertex(barX, barY, 0.0f);
        t.vertex(barX, barY + barHeight, 0.0f);
        t.vertex(barX + progress, barY + barHeight, 0.0f);
        t.vertex(barX + progress, barY, 0.0f);
        t.end();
        

        glEnable(GL_TEXTURE_2D);
    }
    
    this->font->drawShadow(this->title, (screenWidth - this->font->width(this->title)) / 2, screenHeight / 2 - 4 - 16, 0xFFFFFF);
    this->font->drawShadow(this->status, (screenWidth - this->font->width(this->status)) / 2, screenHeight / 2 - 4 + 8, 0xFFFFFF);
    
    glfwSwapBuffers(CrossCraft::instance->window);

#ifdef __EMSCRIPTEN__
    emscripten_sleep(1);
#endif 
}