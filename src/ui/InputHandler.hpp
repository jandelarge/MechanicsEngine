#pragma once
#include "physics/PhysicsWorld.hpp"
#include "raylib.h"
#include <string>
#include <vector>

enum class AppScreen {
    MAIN_MENU = 0,
    STUDY_HUB,
    SETTINGS,
    ABOUT,
    SANDBOX,
    STUDY_LAB
};

enum class ToolType {
    SELECT_MOVE = 0,
    ADD_BLOCK,
    ADD_CIRCLE,
    ADD_SUPPORT,
    ADD_ROPE,
    ADD_SPRING,
    ADD_PULLEY,
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
    BODY_TRAIL_LIFETIME,
    SUPPORT_X,
    SUPPORT_Y,
    SUPPORT_WIDTH,
    SUPPORT_ANGLE,
    ROPE_LENGTH,
    ROPE_STIFFNESS,
    WORLD_GRAVITY,
    WORLD_SUBSTEPS,
    NEW_ROPE_LENGTH,
    WORLD_TRAIL_LIFETIME
};

struct TelemetrySample {
    float time = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    float speed = 0.0f;
    float angVel = 0.0f;
    float normalF = 0.0f;
    float frictionF = 0.0f;
    float ek = 0.0f;
    float ep = 0.0f;
    float etot = 0.0f;
    float theoryVal = 0.0f;
    float simVal = 0.0f;
    float errorPct = 0.0f;
};

struct AppState {
    AppScreen screen = AppScreen::MAIN_MENU;
    AppScreen prevScreen = AppScreen::MAIN_MENU;
    bool shouldExit = false;

    bool paused = true;
    int forceViewMode = 0;
    bool hideAllBodies = false;
    bool showCenterOfMass = false;
    bool showProfiler = false;

    float timeScale = 1.0f;
    bool stepOneFrame = false;

    bool showGraph = false;
    int graphTab = 0;
    int trackedBodyId = -1;
    std::vector<TelemetrySample> telemetryHistory;
    Vector2 graphPos = {-1.0f, -1.0f};
    Vector2 graphSize = {480.0f, 260.0f};
    bool isDraggingGraph = false;
    bool isResizingGraph = false;
    Vector2 graphDragOffset = {0.0f, 0.0f};

    ToolType activeTool = ToolType::SELECT_MOVE;
    bool addMenuOpen = false;

    Vec2 camPos = {-0.8f, 4.3f};
    float camZoom = 1.0f;
    bool isPanning = false;
    Vector2 panStartMouse = {0.0f, 0.0f};
    Vec2 panStartCam = {-0.8f, 4.3f};

    int selectedBodyId = -1;
    int selectedRopeId = -1;

    int hoveredBodyId = -1;
    int hoveredRopeId = -1;
    int hoveredDockBtn = -1;
    int hoveredAddMenuBtn = -1;

    int draggedBodyId = -1;
    int draggedFreeRopeId = -1;

    int creatingSupportBodyId = -1;
    Vec2 supportStartPos = {0.0f, 0.0f};

    int ropeStartBodyId = -1;
    Vec2 ropeStartOffset = {0.0f, 0.0f};
    float newRopeDefaultLength = 2.2f;

    Vec2 mouseWorldPos = {0.0f, 0.0f};
    Vec2 dragOffset = {0.0f, 0.0f};

    EditField editingField = EditField::NONE;
    std::string inputBuffer = "";

    std::string statusMessage = "";
    float statusTimer = 0.0f;
};

class InputHandler {
public:
    static const int INSPECTOR_WIDTH = 300;

    static Rectangle getDockRect(int screenH);
    static Rectangle getDockButtonRect(int index, int screenH);

    static Rectangle getAddMenuRect(int screenH);
    static Rectangle getAddMenuItemRect(int index, int screenH);

    static Rectangle getChronoBarRect(int screenW);
    static Rectangle getGraphRect(const AppState& state, int screenW, int screenH);

    static int findTargetTelemetryBody(const PhysicsWorld& world, const AppState& state);
    static void recordTelemetry(const PhysicsWorld& world, AppState& state);
    static void clearTelemetry(const PhysicsWorld& world, AppState& state);
    static bool exportTelemetryCSV(const AppState& state, const std::string& filename);
    static void showStatus(AppState& state, const std::string& msg, float duration = 3.0f);

    static void update(PhysicsWorld& world, AppState& state, int screenW, int screenH, float ppm);
    static void commitEditField(PhysicsWorld& world, AppState& state);

private:
    static void handleMenuScreens(PhysicsWorld& world, AppState& state, int screenW, int screenH);
    static bool handleChronoBar(PhysicsWorld& world, AppState& state, int screenW);
    static bool handleGraphWindow(PhysicsWorld& world, AppState& state, int screenW, int screenH);
    static bool handleDock(PhysicsWorld& world, AppState& state, int screenH);
    static void handleInspector(PhysicsWorld& world, AppState& state, int screenW, int screenH);
    static void handleKeyboardInput(PhysicsWorld& world, AppState& state);
};