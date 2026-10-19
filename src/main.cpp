#include "raylib.h"
#include "physics/PhysicsWorld.hpp"
#include "ui/InputHandler.hpp"
#include "ui/Renderer.hpp"

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 760, "Симулятор механики");
    SetTargetFPS(60);

    PhysicsWorld world;
    world.groundY = 0.5f;

    int anchorId = world.createBody({0.0f, 7.5f}, 3.5f, 0.2f, 0.0f, true);
    int hangingId = world.createBody({0.0f, 4.0f}, 1.3f, 1.3f, 5.0f, false);
    int freeId = world.createBody({-3.2f, 1.0f}, 1.0f, 1.0f, 2.0f, false);

    world.addRope(anchorId, hangingId, {0.0f, 0.0f}, {0.0f, 0.65f}, 2.4f, 120.0f);
    world.updateForces();

    Renderer renderer;
    renderer.init();

    AppState state;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        InputHandler::update(world, state, GetScreenWidth(), GetScreenHeight(), renderer.ppm, hangingId, freeId);

        if (!state.paused) {
            Vec2 targetPos = state.mouseWorldPos + state.dragOffset;
            world.step(dt, state.draggedBodyId, targetPos);
        }

        renderer.draw(world, state);
    }

    renderer.unload();
    CloseWindow();
    return 0;
}