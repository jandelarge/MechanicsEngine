#pragma once
#include "core/MathVec.hpp"
#include <string>
#include <algorithm>
#include <cmath>
#include <array>

struct Body {
    int id = 0;
    std::string name = "Груз";

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
        mass = std::clamp(mass, 0.1f, 500.0f);
        width = std::clamp(width, 0.4f, 8.0f);
        height = std::clamp(height, 0.2f, 6.0f);

        inertia = (1.0f / 12.0f) * mass * (width * width + height * height);

        if (isStatic) {
            invMass = 0.0f;
            invInertia = 0.0f;
            vel = {0.0f, 0.0f};
            angVel = 0.0f;
            angle = 0.0f;
        } else {
            invMass = 1.0f / mass;
            invInertia = canRotate ? (1.0f / inertia) : 0.0f;
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

    bool containsPoint(const Vec2& pt) const {
        Vec2 local = worldToLocal(pt);
        float pad = isStatic ? 0.08f : 0.0f;
        return (local.x >= -width * 0.5f - pad && local.x <= width * 0.5f + pad &&
                local.y >= -height * 0.5f - pad && local.y <= height * 0.5f + pad);
    }
};