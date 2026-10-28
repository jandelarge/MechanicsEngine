#include "raylib.h"
#include "physics/PhysicsWorld.hpp"
#include "ui/InputHandler.hpp"
#include "ui/Renderer.hpp"

int main() {
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1320, 780, "MechanicsEngine v1.0.0 — Интерактивная цифровая физическая лаборатория");
    SetExitKey(KEY_NULL);

    SetWindowMinSize(1024, 660);
    SetTargetFPS(60);

    PhysicsWorld world;
    world.setupDefaultScene();

    Renderer renderer;
    renderer.init();

    AppState state;
    state.screen = AppScreen::MAIN_MENU;
    state.paused = true;
    InputHandler::recordTelemetry(world, state);

    while (!WindowShouldClose() && !state.shouldExit) {
        float dt = GetFrameTime();
        if (dt > 0.025f) dt = 0.025f;

        InputHandler::update(world, state, GetScreenWidth(), GetScreenHeight(), renderer.ppm);

        bool inSimulationView = (state.screen == AppScreen::SANDBOX || state.screen == AppScreen::STUDY_LAB);
        if (inSimulationView) {
            if (!state.paused) {
                Vec2 targetPos = state.mouseWorldPos + state.dragOffset;
                world.step(dt * state.timeScale, state.draggedBodyId, targetPos, state.draggedFreeRopeId);
                InputHandler::recordTelemetry(world, state);
            } else if (state.stepOneFrame) {
                Vec2 targetPos = state.mouseWorldPos + state.dragOffset;
                world.step(1.0f / 60.0f, state.draggedBodyId, targetPos, state.draggedFreeRopeId);
                InputHandler::recordTelemetry(world, state);
                state.stepOneFrame = false;
            }
        }

        renderer.draw(world, state);
    }

    renderer.unload();
    CloseWindow();
    return 0;
}