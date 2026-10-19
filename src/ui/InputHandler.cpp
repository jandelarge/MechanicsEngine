#include "InputHandler.hpp"
#include "raylib.h"

void InputHandler::update(PhysicsWorld& world, AppState& state, int screenW, int screenH, float ppm, int hangingId, int freeId) {
    world.worldHalfWidth = (screenW * 0.5f) / ppm - 0.1f;
    world.worldTopY = (screenH * 0.85f) / ppm - 0.1f;

    if (IsKeyPressed(KEY_SPACE)) state.paused = !state.paused;
    if (IsKeyPressed(KEY_V)) state.showAllForces = !state.showAllForces;
    if (IsKeyPressed(KEY_R)) {
        state.draggedBodyId = -1;
        world.resetDefaultScene(hangingId, freeId);
    }

    Vector2 mouse = GetMousePosition();
    state.mouseWorldPos = {
        (mouse.x - screenW * 0.5f) / ppm,
        (screenH * 0.85f - mouse.y) / ppm
    };

    state.hoveredBodyId = -1;
    state.hoveredRopeId = -1;

    for (const auto& b : world.bodies) {
        if (!b.isStatic && b.containsPoint(state.mouseWorldPos)) {
            state.hoveredBodyId = b.id;
            break;
        }
    }

    if (state.hoveredBodyId == -1 && state.draggedBodyId == -1) {
        const float ROPE_HITBOX_RADIUS = 0.22f;
        for (const auto& r : world.ropes) {
            Vec2 pA = world.bodies[r.idA].pos + r.offsetA;
            Vec2 pB = world.bodies[r.idB].pos + r.offsetB;
            if (Vec2::distToSegment(state.mouseWorldPos, pA, pB) <= ROPE_HITBOX_RADIUS) {
                state.hoveredRopeId = r.id;
                break;
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && state.hoveredBodyId != -1) {
        state.draggedBodyId = state.hoveredBodyId;
        state.dragOffset = world.bodies[state.draggedBodyId].pos - state.mouseWorldPos;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (state.draggedBodyId >= 0 && state.draggedBodyId < (int)world.bodies.size()) {
            world.bodies[state.draggedBodyId].vel *= 0.15f;
        }
        state.draggedBodyId = -1;
    }
}