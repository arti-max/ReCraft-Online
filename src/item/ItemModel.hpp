#pragma once
#include "model/ModelPart.hpp"

class ItemModel {
private:
    ModelPart* model = new ModelPart(0, 0);
public:

    ItemModel(int textureId) {
        float x = -2.0f;
        float y = -2.0f;
        float z = -2.0f;
        this->model->quads.clear();
        NewVertex v1(x, y, z, 0.0f, 0.0f);
        NewVertex v2(2.0f, y, z, 0.0f, 8.0f);
        NewVertex v3(2.0f, 2.0f, z, 8.0f, 8.0f);
        NewVertex v4(x, 2.0f, z, 8.0f, 0.0f);
        NewVertex v5(x, y, 2.0f, 0.0f, 0.0f);
        NewVertex v6(2.0f, y, 2.0f, 0.0f, 8.0f);
        NewVertex v7(2.0f, 2.0f, 2.0f, 8.0f, 8.0f);
        NewVertex v8(x, 2.0f, 2.0f, 8.0f, 0.0f);
        float uuo = 0.25f;
        float vvo = 0.25f;
        float u1 = ((float)(textureId % 16) + (1.0f - uuo)) / 16.0f;
        float v1_c = ((float)(textureId / 16) + (1.0f - vvo)) / 16.0f;
        float u0 = ((float)(textureId % 16) + uuo) / 16.0f;
        float v0_c = ((float)(textureId / 16) + vvo) / 16.0f;
        this->model->quads.push_back(Quad({v6, v2, v3, v7}, u1, v1_c, u0, v0_c));
        this->model->quads.push_back(Quad({v1, v5, v8, v4}, u1, v1_c, u0, v0_c));
        this->model->quads.push_back(Quad({v6, v5, v1, v2}, u1, v1_c, u0, v0_c));
        this->model->quads.push_back(Quad({v3, v4, v8, v7}, u1, v1_c, u0, v0_c));
        this->model->quads.push_back(Quad({v2, v1, v4, v3}, u1, v1_c, u0, v0_c));
        this->model->quads.push_back(Quad({v5, v6, v7, v8}, u1, v1_c, u0, v0_c));
    }

    virtual ~ItemModel() = default;

    void generateList() {
        this->model->render(0.0625f);
    }
};