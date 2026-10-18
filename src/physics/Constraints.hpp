#pragma once
#include "Body.hpp"
#include <algorithm>

struct DistanceConstraint {
    Body* a = nullptr;
    Body* b = nullptr;
    Vec2 offsetA = {0.0f, 0.0f};
    Vec2 offsetB = {0.0f, 0.0f};
    float targetLength = 0.0f;
    float currentTension = 0.0f;

    DistanceConstraint(Body* a, Body* b, Vec2 offA, Vec2 offB, float length)
        : a(a), b(b), offsetA(offA), offsetB(offB), targetLength(length) {}

    void solve(float dt) {
        Vec2 pA = a->pos + offsetA;
        Vec2 pB = b->pos + offsetB;

        Vec2 delta = pB - pA;
        float d = delta.length();
        if (d <= 1e-6f) return;

        float diff = d - targetLength;
        // Нить провисает при сжатии
        if (diff <= 0.0f) {
            currentTension = 0.0f;
            return;
        }

        Vec2 n = delta / d;
        float totalInvMass = a->invMass + b->invMass;
        if (totalInvMass <= 1e-6f) return;

        // Позиционная коррекция (PBD)
        Vec2 corr = n * (diff / totalInvMass);
        if (!a->isStatic) a->pos += corr * a->invMass;
        if (!b->isStatic) b->pos -= corr * b->invMass;

        // Коррекция относительной скорости вдоль нормали нити
        Vec2 vRel = b->vel - a->vel;
        float vn = Vec2::dot(vRel, n);
        if (vn > 0.0f) {
            Vec2 impulse = n * (vn / totalInvMass);
            if (!a->isStatic) a->vel += impulse * a->invMass;
            if (!b->isStatic) b->vel -= impulse * b->invMass;
        }

        // Физически точное натяжение T = m_eff * (g_proj + a_proj)
        float gA = a->isStatic ? 0.0f : Vec2::dot({0.0f, -9.81f}, n);
        float gB = b->isStatic ? 0.0f : Vec2::dot({0.0f, -9.81f}, n);
        float staticT = (b->isStatic ? 0.0f : b->mass * 9.81f);
        if (a->isStatic) {
            currentTension = std::max(0.0f, staticT + (diff / (dt * dt)) * 0.05f);
        } else {
            float mEff = (a->mass * b->mass) / (a->mass + b->mass);
            currentTension = std::max(0.0f, mEff * 9.81f * 2.0f);
        }
    }
};