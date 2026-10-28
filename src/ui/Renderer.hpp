#pragma once
#include "raylib.h"
#include "physics/PhysicsWorld.hpp"
#include "InputHandler.hpp"

class Renderer {
public:
    float basePpm = 70.0f;
    float ppm = 70.0f;
    Vec2 camPos = {-0.8f, 4.3f};

    Font fontSmall{};
    Font fontMain{};
    Font fontBold{};
    Font fontTitle{};

    void init();
    void unload();
    void draw(const PhysicsWorld& world, const AppState& state);

    Vector2 worldToScreen(Vec2 wPos, int sw, int sh) const;

private:
    void drawMainMenu(int sw, int sh) const;
    void drawStudyHub(const PhysicsWorld& world, int sw, int sh) const;
    void drawSettingsScreen(const PhysicsWorld& world, const AppState& state, int sw, int sh) const;
    void drawAboutScreen(int sw, int sh) const;

    void drawInfiniteGridAndFloor(const PhysicsWorld& world, int sw, int sh) const;
    void drawTrails(const PhysicsWorld& world, const AppState& state, int sw, int sh) const;
    void drawPulleys(const PhysicsWorld& world, const AppState& state, int sw, int sh) const;
    void drawCenterOfMass(const PhysicsWorld& world, const AppState& state, int sw, int sh) const;
    void drawProfilerOverlay(const PhysicsWorld& world, const AppState& state) const;
    void drawChronoBar(const PhysicsWorld& world, const AppState& state, int sw) const;
    void drawOscilloscope(const PhysicsWorld& world, const AppState& state, int sw, int sh) const;
    void drawLiquidDock(int sh, const AppState& state) const;
    void drawDockIcon(int index, Rectangle btnRect, bool active, bool paused, Color iconCol) const;
    void drawCreationIcon(int itemIndex, Vector2 center, Color col, bool active) const;
    void drawSpringCoil(Vector2 sA, Vector2 sB, Color col, float thickness) const;
    void drawInspector(const PhysicsWorld& world, const AppState& state, int sw, int sh) const;
    void drawValueInputRow(float x, float y, const char* label, const char* currentValText,
                           bool editing, const std::string& inputBuffer) const;
    void drawHatchedSupport(const Body& b, int sw, int sh, bool highlighted, bool unlocked) const;
    void drawRotatedBlock(const Body& b, Vector2 scr, float bw, float bh, bool isSelected, bool isActive) const;
    void drawVectorArrow(Vector2 start, Vector2 end, Color color, const char* label, Vector2 labelOffset = {8.0f, -8.0f}) const;
    void drawRopeTooltip(const RopeLink& r, Vector2 midScreen) const;
    void drawButton(Rectangle r, const char* text, bool active, Color accent) const;
};