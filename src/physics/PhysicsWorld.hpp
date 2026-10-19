#pragma once
#include "Body.hpp"
#include "Rope.hpp"
#include <vector>

class PhysicsWorld {
public:
    const float G = 9.81f;
    std::vector<Body> bodies;
    std::vector<RopeLink> ropes;

    float groundY = 0.5f;
    float worldHalfWidth = 8.5f;
    float worldTopY = 9.0f;

    int createBody(Vec2 pos, float w, float h, float mass, bool isStatic = false);
    int addRope(int idA, int idB, Vec2 offA, Vec2 offB, float length, float stiffness = 120.0f);

    void resetDefaultScene(int hangingId, int freeId);
    void step(float dt, int draggedId, const Vec2& targetBodyPos);
    void updateForces();

private:
    void solveRopes(float subDt, int draggedId);
    void solveCollisions(float subDt, int draggedId);
    void enforceHardLimits(int draggedId);
};