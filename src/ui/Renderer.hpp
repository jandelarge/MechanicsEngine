#pragma once
#include "raylib.h"
#include "physics/PhysicsWorld.hpp"
#include "InputHandler.hpp"

class Renderer {
public:
    float ppm = 70.0f;
    Font fontMain{};
    Font fontBold{};

    void init();
    void unload();
    void draw(const PhysicsWorld& world, const AppState& state);

    Vector2 worldToScreen(Vec2 wPos, int sw, int sh) const;

private:
    void drawHatchedSupport(Vector2 pos, float width, float height) const;
    void drawVectorArrow(Vector2 start, Vector2 end, Color color, const char* label, Vector2 labelOffset = {8.0f, -8.0f}) const;
    void drawRopeTooltip(const RopeLink& r, Vector2 midScreen) const;
};