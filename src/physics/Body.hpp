#pragma once
#include "core/MathVec.hpp"
#include <string>

struct Body {
    int id = 0;
    std::string name = "body";

    Vec2 pos = {0.0f, 0.0f};
    Vec2 vel = {0.0f, 0.0f};

    float width = 1.0f;
    float height = 1.0f;
    float mass = 1.0f;
    float invMass = 1.0f;
    float friction = 0.8f;
    bool isStatic = false;

    int supportedBy = -1;       // -1: воздух, -2: пол, >= 0: id нижнего тела
    float normalForce = 0.0f;   // Сила реакции опоры N (вверх)
    float topLoadForce = 0.0f;  // Сила давления верхних тел P (вниз)

    Body(int id, Vec2 pos, float w, float h, float mass, bool isStatic = false)
        : id(id), pos(pos), width(w), height(h), mass(mass), isStatic(isStatic)
    {
        invMass = isStatic ? 0.0f : (1.0f / mass);
    }

    bool containsPoint(const Vec2& pt) const {
        return (pt.x >= pos.x - width * 0.5f && pt.x <= pos.x + width * 0.5f &&
                pt.y >= pos.y - height * 0.5f && pt.y <= pos.y + height * 0.5f);
    }
};