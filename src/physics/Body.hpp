#pragma once
#include "core/MathVec.hpp"
#include <string>
#include <algorithm>
#include <cmath>
#include <array>

enum class ShapeType { BOX = 0, CIRCLE};


struct Body {
    int id = 0;
    std::string name = "Груз";

    ShapeType shape = ShapeType::BOX;
    float radius = 0.5f;

    Vec2 pos = {0.0f, 0.0f};
    Vec2 vel = {0.0f, 0.0f};

    float angle = 0.0f;
    float angVel = 0.0f;

    float width = 1.0f;
    float height = 1.0f;
    float mass = 1.0f;
    float invMass = 1.0f;
    float inertia = 1.0f;
    float invInertia = 1.0f;

    float friction = 0.6f;
    bool isStatic = false;
    bool supportLocked = true;
    bool canRotate = true;


    int supportedBy = -1;
    float normalForce = 0.0f;
    float topLoadForce = 0.0f;
    float frictionForce = 0.0f;

    Body() = default;
    Body(int id, Vec2 pos, float w, float h, float m, bool isStatic = false)
        : id(id), pos(pos), width(w), height(h), mass(m), isStatic(isStatic)
    {
        updateMassData();
    }

    void updateMassData() {
        if (isStatic) {
            inertia = 0.0f;
            return;
        }
        if (!canRotate) {
            inertia = 0.0f;
            return;
        }

        if (shape == ShapeType::CIRCLE) {
            inertia =  0.5f * mass * (radius * radius);
        } else {
        inertia = (1.0f / 12.0f)*mass*(width*width + height * height);
        }
    }

    void setMass(float m) {
        mass = std::clamp(m, 0.1f, 500.0f);
        updateMassData();
    }

    Vec2 axisX() const { return {std::cos(angle), std::sin(angle)}; }
    Vec2 axisY() const { return {-std::sin(angle), std::cos(angle)}; }

    std::array<Vec2, 4> getVertices() const {
        Vec2 ux = axisX() * (width * 0.5f);
        Vec2 uy = axisY() * (height * 0.5f);
        return {
            pos - ux - uy,
            pos + ux - uy,
            pos + ux + uy,
            pos - ux + uy
        };
    }

    Vec2 localToWorld(const Vec2& localOffset) const {
        return pos + localOffset.rotated(angle);
    }

    Vec2 worldToLocal(const Vec2& worldPt) const {
        return (worldPt - pos).rotated(-angle);
    }

    bool containsPoint(Vec2 p) const {
        if (shape == ShapeType::CIRCLE) {
            return (p-pos).length() <= radius;
        }

        Vec2 d = p - pos;
        float c = std::cos(-angle);
        float s = std::sin(-angle);
        Vec2 localP = {d.x *c - d.y *s, d.x * s + d.y * c};

        return (std::abs(localP.x)<= width *0.5f && std::abs(localP.y)<= height * 0.5f);
    }
};