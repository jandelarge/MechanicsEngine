#pragma once
#include "core/MathVec.hpp"
#include <algorithm>

struct RopeLink {
    int id = 0;
    int idA = -1;
    int idB = -1;
    Vec2 offsetA = {0.0f, 0.0f};
    Vec2 offsetB = {0.0f, 0.0f};

    float restLength = 0.0f;      // Свободная длина L0 (м)
    float currentLength = 0.0f;   // Текущая длина L (м)
    float maxStretch = 0.85f;     // Предел удлинения (защита от продавливания)
    float stiffness = 180.0f;     // Коэффициент жесткости k (Н/м)
    float damping = 14.0f;        // Коэффициент затухания колебаний (Н*с/м)

    float tension = 0.0f;         // Текущая сила натяжения T (Н)
    Vec2 dir = {0.0f, 1.0f};      // Единичный вектор вдоль нити от B к A

    RopeLink(int id, int a, int b, Vec2 offA, Vec2 offB, float length, float k = 180.0f)
        : id(id), idA(a), idB(b), offsetA(offA), offsetB(offB),
          restLength(length), currentLength(length), stiffness(k) {}

    float extension() const {
        return std::max(0.0f, currentLength - restLength);
    }
};