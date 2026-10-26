#include "raylib.h"
#include "physics/PhysicsWorld.hpp"
#include "ui/InputHandler.hpp"
#include "ui/Renderer.hpp"

int main() {
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1320, 780, "Собака ебака");
    SetExitKey(KEY_NULL);

    SetWindowMinSize(960, 600);
    SetTargetFPS(60);

    PhysicsWorld world;
    world.setupDefaultScene();

    Renderer renderer;
    renderer.init();

    AppState state;
    state.paused = true;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.025f) dt = 0.025f;

        InputHandler::update(world, state, GetScreenWidth(), GetScreenHeight(), renderer.ppm);

        if (!state.paused) {
            Vec2 targetPos = state.mouseWorldPos + state.dragOffset;
            world.step(dt, state.draggedBodyId, targetPos, state.draggedFreeRopeId);
        }

        renderer.draw(world, state);
    }

    renderer.unload();
    CloseWindow();
    return 0;
}