#pragma once
#include "core/MathVec.hpp"
#include <algorithm>

struct RopeLink {
    int id = 0;
    int idA = -1;
    int idB = -1;                 // -1 = свободный конец нити
    Vec2 offsetA = {0.0f, 0.0f};
    Vec2 offsetB = {0.0f, 0.0f};

    Vec2 freePos = {0.0f, 0.0f};  // координата свободного конца, если idB == -1
    Vec2 freeVel = {0.0f, 0.0f};

    bool isRigid = false;         // true: нерастяжимая, false: упругая
    float restLength = 2.0f;      // свободная длина
    float currentLength = 2.0f;   // текущая длина
    float maxStretch = 4.0f;      // запас прочности
    float stiffness = 180.0f;     // коэффициент жесткости
    float damping = 18.0f;        // коэффициент демпфирования

    float tension = 0.0f;         // сила натяжения
    Vec2 dir = {0.0f, 1.0f};      // единичный вектор вдоль нити к точке крепления

    RopeLink() = default;
    RopeLink(int id, int a, int b, Vec2 offA, Vec2 offB, float length, float k = 180.0f, bool rigid = false)
        : id(id), idA(a), idB(b), offsetA(offA), offsetB(offB),
          isRigid(rigid), restLength(length), currentLength(length), stiffness(k) {}

    float extension() const {
        if (isRigid || idB == -1) return 0.0f;
        return std::max(0.0f, currentLength - restLength);
    }
};