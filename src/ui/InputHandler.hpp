#pragma once
#include "physics/PhysicsWorld.hpp"
#include "raylib.h"
#include <string>

enum class ToolType {
    SELECT_MOVE = 0,
    ADD_BLOCK,
    ADD_CIRCLE,
    ADD_SUPPORT,
    ADD_ROPE,
    ADD_SPRING,
    DELETE_OBJ
};

enum class EditField {
    NONE = 0,
    BODY_MASS,
    BODY_FRICTION,
    BODY_RESTITUTION,
    BODY_WIDTH,
    BODY_HEIGHT,
    BODY_RADIUS,
    BODY_FORCE_X,
    SUPPORT_X,
    SUPPORT_Y,
    SUPPORT_WIDTH,
    SUPPORT_ANGLE,
    ROPE_LENGTH,
    ROPE_STIFFNESS,
    WORLD_GRAVITY,
    WORLD_SUBSTEPS,
    NEW_ROPE_LENGTH
};

struct AppState {
    bool paused = true;
    int forceViewMode = 0; // 0: при наведении, 1: все, 2: скрыты

    ToolType activeTool = ToolType::SELECT_MOVE;
    bool addMenuOpen = false;

    int selectedBodyId = -1;
    int selectedRopeId = -1;

    int hoveredBodyId = -1;
    int hoveredRopeId = -1;
    int hoveredDockBtn = -1;
    int hoveredAddMenuBtn = -1;

    int draggedBodyId = -1;
    int draggedFreeRopeId = -1;

    // Состояние растягивания новой опоры зажатой ЛКМ
    int creatingSupportBodyId = -1;
    Vec2 supportStartPos = {0.0f, 0.0f};

    int ropeStartBodyId = -1;
    Vec2 ropeStartOffset = {0.0f, 0.0f};
    float newRopeDefaultLength = 2.2f;

    Vec2 mouseWorldPos = {0.0f, 0.0f};
    Vec2 dragOffset = {0.0f, 0.0f};

    EditField editingField = EditField::NONE;
    std::string inputBuffer = "";
};

class InputHandler {
public:
    static const int INSPECTOR_WIDTH = 300;

    static Rectangle getDockRect(int screenH);
    static Rectangle getDockButtonRect(int index, int screenH);

    static Rectangle getAddMenuRect(int screenH);
    static Rectangle getAddMenuItemRect(int index, int screenH);

    static void update(PhysicsWorld& world, AppState& state, int screenW, int screenH, float ppm);
    static void commitEditField(PhysicsWorld& world, AppState& state);

private:
    static bool handleDock(PhysicsWorld& world, AppState& state, int screenH);
    static void handleInspector(PhysicsWorld& world, AppState& state, int screenW, int screenH);
    static void handleKeyboardInput(PhysicsWorld& world, AppState& state);
};