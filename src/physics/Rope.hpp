#pragma once
#include "core/MathVec.hpp"
#include <algorithm>
#include <cmath>

struct RopeLink {
    int id = 0;
    int idA = -1;
    int idB = -1;                 // -1 = свободный конец
    Vec2 offsetA = {0.0f, 0.0f};
    Vec2 offsetB = {0.0f, 0.0f};

    Vec2 freePos = {0.0f, 0.0f};  // координата свободного конца, если idB == -1
    Vec2 freeVel = {0.0f, 0.0f};

    bool isSpring = false;        // true: двусторонняя пружина (сжатие + растяжение)
    bool isRigid = false;         // true: жесткий стержень / нерастяжимая нить
    float restLength = 2.0f;      // свободная длина L0
    float currentLength = 2.0f;   // текущая длина L
    float maxStretch = 4.0f;      // запас растяжения
    float stiffness = 180.0f;     // коэффициент жесткости k
    float damping = 18.0f;        // коэффициент демпфирования

    float tension = 0.0f;         // модуль силы упругости / натяжения
    Vec2 dir = {0.0f, 1.0f};      // единичный вектор вдоль линии связи

    RopeLink() = default;
    RopeLink(int id, int a, int b, Vec2 offA, Vec2 offB, float length, float k = 180.0f, bool rigid = false, bool spring = false)
        : id(id), idA(a), idB(b), offsetA(offA), offsetB(offB),
          isSpring(spring), isRigid(rigid), restLength(length), currentLength(length), stiffness(k) {}

    // Знаковая деформация: > 0 растяжение, < 0 сжатие
    float signedExtension() const {
        if (isRigid || idB == -1) return 0.0f;
        if (isSpring) return currentLength - restLength;
        return std::max(0.0f, currentLength - restLength);
    }

    float extension() const {
        return std::abs(signedExtension());
    }

    // Потенциальная энергия упругости E = k * dx^2 / 2
    float potentialEnergy() const {
        if (isRigid || idB == -1) return 0.0f;
        float dx = signedExtension();
        return 0.5f * stiffness * dx * dx;
    }
};