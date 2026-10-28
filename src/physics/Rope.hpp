#pragma once
#include "core/MathVec.hpp"
#include <algorithm>
#include <cmath>

struct RopeLink {
    int id = 0;
    int idA = -1;
    int idB = -1;
    Vec2 offsetA = {0.0f, 0.0f};
    Vec2 offsetB = {0.0f, 0.0f};

    Vec2 freePos = {0.0f, 0.0f};
    Vec2 freeVel = {0.0f, 0.0f};

    bool isSpring = false;
    bool isRigid = false;
    float restLength = 2.0f;
    float currentLength = 2.0f;
    float maxStretch = 4.0f;
    float stiffness = 180.0f;
    float damping = 18.0f;

    float tension = 0.0f;
    Vec2 dir = {0.0f, 1.0f};

    RopeLink() = default;
    RopeLink(int id, int a, int b, Vec2 offA, Vec2 offB, float length, float k = 180.0f, bool rigid = false, bool spring = false)
        : id(id), idA(a), idB(b), offsetA(offA), offsetB(offB),
          isSpring(spring), isRigid(rigid), restLength(length), currentLength(length), stiffness(k) {}

    float signedExtension() const {
        if (isRigid || idB == -1) return 0.0f;
        if (isSpring) return currentLength - restLength;
        return std::max(0.0f, currentLength - restLength);
    }

    float extension() const {
        return std::abs(signedExtension());
    }

    float potentialEnergy() const {
        if (isRigid || idB == -1) return 0.0f;
        float dx = signedExtension();
        return 0.5f * stiffness * dx * dx;
    }
};

struct PulleyLink {
    int id = 0;
    int idA = -1;
    int idB = -1;
    Vec2 offsetA = {0.0f, 0.0f};
    Vec2 offsetB = {0.0f, 0.0f};
    Vec2 wheelPos = {0.0f, 6.8f};
    float radius = 0.75f;
    float totalLength = 6.2f;
    float wheelAngle = 0.0f;
    float tension = 0.0f;
    Vec2 dirA = {0.0f, 1.0f};
    Vec2 dirB = {0.0f, 1.0f};

    Vec2 anchorLeft() const  { return {wheelPos.x - radius, wheelPos.y}; }
    Vec2 anchorRight() const { return {wheelPos.x + radius, wheelPos.y}; }
};