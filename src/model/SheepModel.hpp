#pragma once
#include "model/ModelPart.hpp"
#include "model/PigModel.hpp"

class SheepModel : public PigModel {
public:

    SheepModel() : PigModel(12, 0.0f) {
        this->head = new ModelPart(0, 0);
        this->head->addBox(-3.0f, -4.0f, -6.0f, 6, 6, 8, 0.0f);
        this->head->setPosition(0.0f, 6.0f, -8.0f);
        this->body = new ModelPart(28, 8);
        this->body->addBox(-4.0f, -10.0f, -7.0f, 8, 16, 6, 0.0f);
        this->body->setPosition(0.0f, 5.0f, 2.0f);
    }

};