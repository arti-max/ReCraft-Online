#include "render/Tessellator.hpp"
#include "GL/gl.h"
#include <stdio.h>

Tessellator& Tessellator::getInstance() {
    static Tessellator instance;
    return instance;
}

Tessellator::Tessellator() {
    vertices = 0;
    p = 0; 
    len = VERTEX_SIZE;
    u = 0.0f; v = 0.0f;
    r = 1.0f; g = 1.0f; b = 1.0f;
    nx = 0.0f; ny = 1.0f; nz = 0.0f;
    hasColor = false;
    hasTexture = false;
    noColor = false;
    hasNormal = false;

    buffer.resize(MAX_FLOATS);

    int maxQuads = MAX_FLOATS / (VERTEX_SIZE * 4);
    indexBuffer.reserve(maxQuads * 6);
}

void Tessellator::end() {
    // printf("FLUSH GEOMETRY %i, p: %i, len: %i\n", this->vertices, this->p, this->len);
    if (this->vertices == 0) {
        return;
    }

    int stride = VERTEX_SIZE * sizeof(float);
    float* data = buffer.data();

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, data+0);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, stride, data+3);

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(3, GL_FLOAT, stride, data+5);
    
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, stride, data+8);

    glDrawElements(GL_TRIANGLES, indexBuffer.size(), GL_UNSIGNED_INT, indexBuffer.data());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    clear();
}

void Tessellator::begin() {
    clear();
    hasColor = false;
    hasTexture = false;
    noColor = false;
    hasNormal = false;
    
    u = 0.0f; v = 0.0f;
    r = 1.0f; g = 1.0f; b = 1.0f;
    nx = 0.0f, ny = 1.0f, nz = 0.0f;
}

void Tessellator::clear() {
    vertices = 0;
    p = 0;
    indexBuffer.clear();
}

void Tessellator::texture(float u, float v) {
    this->hasTexture = true;
    this->u = u;
    this->v = v;
}

void Tessellator::color(float r, float g, float b) {
    if (!noColor) {
        this->hasColor = true;
        this->r = r;
        this->g = g;
        this->b = b;
    }
}

void Tessellator::normal(float x, float y, float z) {
    this->hasNormal = true;
    this->nx = x;
    this->ny = y;
    this->nz = z;
}

void Tessellator::vertex(float x, float y, float z) {
    // printf("VERTEX %f, %f, %f\n", x, y, z);
    buffer[p++] = x;
    buffer[p++] = y;
    buffer[p++] = z;
    buffer[p++] = u;
    buffer[p++] = v;
    buffer[p++] = r;
    buffer[p++] = g;
    buffer[p++] = b;
    buffer[p++] = nx;
    buffer[p++] = ny;
    buffer[p++] = nz;

    this->vertices++;

    if (vertices % 4 == 0 ) {
        unsigned int baseIdx = vertices - 4;
        indexBuffer.push_back(baseIdx + 0);
        indexBuffer.push_back(baseIdx + 1);
        indexBuffer.push_back(baseIdx + 2);

        indexBuffer.push_back(baseIdx + 0);
        indexBuffer.push_back(baseIdx + 2);
        indexBuffer.push_back(baseIdx + 3);
    }

    if (p >= MAX_FLOATS - VERTEX_SIZE * 4) {
        end();
    }
}

void Tessellator::vertexUV(float x, float y, float z, float u, float v) {
    this->texture(u, v);
    this->vertex(x, y, z);
}

void Tessellator::_noColor() {
    this->noColor = true;
}