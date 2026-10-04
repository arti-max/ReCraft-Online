#pragma once
#include <vector>
#include <GLFW/glfw3.h>
#include <GL/gl.h>

class Tessellator {
private:
    Tessellator();
    Tessellator(const Tessellator&) = delete;
    Tessellator& operator=(const Tessellator&) = delete;

    static const int VERTEX_SIZE = 11;
    std::vector<unsigned int> indexBuffer;
    std::vector<float> buffer;
    int vertices = 0;
    int p = 0;
    int len = 0;
    float u = 0.0f, v = 0.0f;
    float r = 0, g = 0, b = 0;
    float nx = 0, ny = 0, nz = 0;
    bool hasTexture = false;
    bool hasColor = false;
    bool hasNormal = false;
    bool noColor = false;
public:
    static const int MAX_FLOATS = 524288;

    static Tessellator& getInstance();

    void begin();
    void end();
    void clear();

    void vertex(float x, float y, float z);
    void texture(float u, float v);
    void color (float r, float g, float b);
    void normal(float x, float y, float z);
    void vertexUV(float x, float y, float z, float u, float v);
    void _noColor();

    std::vector<float>& getBuffer() { return buffer; }
    int getVertexCount() const { return vertices; }
    int getStride() const { return len * sizeof(float); }
    bool getHasTexture() const { return hasTexture; }
    bool getHasColor() const { return hasColor; }
    bool getHasNormal() const { return hasNormal; }
};