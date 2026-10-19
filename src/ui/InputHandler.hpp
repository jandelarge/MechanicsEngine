#pragma once
#include "physics/PhysicsWorld.hpp"

struct AppState {
    bool paused = false;
    bool showAllForces = false;
    int hoveredBodyId = -1;
    int hoveredRopeId = -1;
    int draggedBodyId = -1;
    Vec2 mouseWorldPos = {0.0f, 0.0f};
    Vec2 dragOffset = {0.0f, 0.0f};
};

class InputHandler {
public:
    static void update(PhysicsWorld& world, AppState& state, int screenW, int screenH, float ppm, int hangingId, int freeId);
};