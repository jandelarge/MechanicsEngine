#include "InputHandler.hpp"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

static const float BASE_PPM = 70.0f;

static bool PointInRect(Vector2 p, Rectangle r) {
    return (p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height);
}

static bool CheckInputBoxClick(Vector2 m, float x, float y) {
    float by = y + 20.0f;
    return PointInRect(m, {x, by, 268.0f, 28.0f});
}

Rectangle InputHandler::getDockRect(int screenH) {
    // 7 кнопок: Пуск, Сброс | [1] Курсор, [2] Создать, [3] Осциллограф, [4] Глазик, [5] Мусорка
    return {20.0f, (float)screenH - 78.0f, 382.0f, 58.0f};
}

Rectangle InputHandler::getDockButtonRect(int index, int screenH) {
    Rectangle dock = getDockRect(screenH);
    float btnSize = 44.0f;
    float y = dock.y + (dock.height - btnSize) * 0.5f;
    float x = dock.x + 8.0f;

    for (int i = 0; i < index; ++i) {
        x += btnSize + 6.0f;
        if (i == 1) x += 12.0f;
    }
    return {x, y, btnSize, btnSize};
}

Rectangle InputHandler::getAddMenuRect(int screenH) {
    Rectangle dock = getDockRect(screenH);
    return {dock.x, dock.y - 70.0f, 490.0f, 62.0f};
}

Rectangle InputHandler::getAddMenuItemRect(int index, int screenH) {
    Rectangle menu = getAddMenuRect(screenH);
    float itemW = 90.0f;
    float itemH = 48.0f;
    float x = menu.x + 8.0f + index * (itemW + 6.0f);
    float y = menu.y + (menu.height - itemH) * 0.5f;
    return {x, y, itemW, itemH};
}

Rectangle InputHandler::getChronoBarRect(int screenW) {
    float canvasW = (float)(screenW - INSPECTOR_WIDTH);
    float barW = 432.0f;
    // Строго по центру рабочей области холста
    float x = (canvasW - barW) * 0.5f;
    return {x, 14.0f, barW, 40.0f};
}

Rectangle InputHandler::getGraphRect(const AppState& state, int screenW, int screenH) {
    float canvasW = (float)(screenW - INSPECTOR_WIDTH);
    float gw = std::clamp(state.graphSize.x, 380.0f, std::max(380.0f, canvasW - 24.0f));
    float gh = std::clamp(state.graphSize.y, 190.0f, std::max(190.0f, (float)screenH - 24.0f));

    float x = (state.graphPos.x < 0.0f) ? (canvasW - gw - 20.0f) : state.graphPos.x;
    float y = (state.graphPos.y < 0.0f) ? ((float)screenH - gh - 20.0f) : state.graphPos.y;

    x = std::clamp(x, 12.0f, std::max(12.0f, canvasW - gw - 12.0f));
    y = std::clamp(y, 12.0f, std::max(12.0f, (float)screenH - gh - 12.0f));
    return {x, y, gw, gh};
}

int InputHandler::findTargetTelemetryBody(const PhysicsWorld& world, const AppState& state) {
    if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
        if (!world.bodies[state.selectedBodyId].isStatic) {
            return state.selectedBodyId;
        }
    }
    if (state.trackedBodyId >= 0 && state.trackedBodyId < (int)world.bodies.size()) {
        if (!world.bodies[state.trackedBodyId].isStatic) {
            return state.trackedBodyId;
        }
    }
    for (const auto& b : world.bodies) {
        if (!b.isStatic) return b.id;
    }
    return -1;
}

void InputHandler::recordTelemetry(const PhysicsWorld& world, AppState& state) {
    int targetId = findTargetTelemetryBody(world, state);
    if (targetId < 0 || targetId >= (int)world.bodies.size()) {
        state.trackedBodyId = -1;
        return;
    }

    if (state.trackedBodyId != targetId) {
        state.trackedBodyId = targetId;
        state.telemetryHistory.clear();
    }

    const Body& b = world.bodies[targetId];
    float speed = b.vel.length();

    // Точная нижняя точка тела и фильтрация микро-зазора коллизии (чтобы на полу Ep был строго 0.0 Дж)
    float minY = b.pos.y - b.radius;
    if (b.shape == ShapeType::BOX) {
        auto verts = b.getVertices();
        minY = std::min({verts[0].y, verts[1].y, verts[2].y, verts[3].y});
    }
    float hAboveFloor = minY - world.groundY;
    if (hAboveFloor < 0.008f) hAboveFloor = 0.0f;

    float ek = 0.5f * b.mass * (speed * speed) + 0.5f * b.inertia * (b.angVel * b.angVel);
    if (ek < 0.005f) ek = 0.0f;

    float ep = b.mass * world.G * hAboveFloor;
    for (const auto& r : world.ropes) {
        if (r.idA == targetId || r.idB == targetId) {
            ep += r.potentialEnergy();
        }
    }
    if (ep < 0.005f) ep = 0.0f;

    TelemetrySample smp;
    smp.time = world.simTime;
    smp.x = b.pos.x;
    smp.y = b.pos.y;
    smp.speed = speed;
    smp.angVel = b.angVel;
    smp.normalF = b.normalForce;
    smp.frictionF = b.frictionForce;
    smp.ek = ek;
    smp.ep = ep;
    smp.etot = ek + ep;

    if (!state.telemetryHistory.empty() && std::abs(state.telemetryHistory.back().time - smp.time) < 1e-5f) {
        state.telemetryHistory.back() = smp;
    } else {
        state.telemetryHistory.push_back(smp);
        if (state.telemetryHistory.size() > 600) {
            state.telemetryHistory.erase(state.telemetryHistory.begin());
        }
    }
}

void InputHandler::clearTelemetry(const PhysicsWorld& world, AppState& state) {
    state.telemetryHistory.clear();
    recordTelemetry(world, state);
}

void InputHandler::commitEditField(PhysicsWorld& world, AppState& state) {
    if (state.editingField == EditField::NONE) return;
    if (!state.inputBuffer.empty()) {
        float val = std::strtof(state.inputBuffer.c_str(), nullptr);

        switch (state.editingField) {
            case EditField::BODY_MASS:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].setMass(std::clamp(val, 0.1f, 500.0f));
                }
                break;
            case EditField::BODY_FRICTION:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].friction = std::clamp(val, 0.0f, 1.0f);
                }
                break;
            case EditField::BODY_RESTITUTION:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].restitution = std::clamp(val, 0.0f, 0.95f);
                }
                break;
            case EditField::BODY_WIDTH:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].width = std::clamp(val, 0.4f, 10.0f);
                    world.bodies[state.selectedBodyId].updateMassData();
                }
                break;
            case EditField::BODY_HEIGHT:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].height = std::clamp(val, 0.4f, 10.0f);
                    world.bodies[state.selectedBodyId].updateMassData();
                }
                break;
            case EditField::BODY_RADIUS:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].setRadius(val);
                    world.resolveEditorOverlap(state.selectedBodyId);
                }
                break;
            case EditField::BODY_FORCE_X:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].appliedForceX = std::clamp(val, -200.0f, 200.0f);
                }
                break;
            case EditField::SUPPORT_X:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].pos.x = std::clamp(val, -500.0f, 500.0f);
                }
                break;
            case EditField::SUPPORT_Y:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].pos.y = std::clamp(val, 0.1f, 500.0f);
                }
                break;
            case EditField::SUPPORT_WIDTH:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].width = std::clamp(val, 0.5f, 30.0f);
                }
                break;
            case EditField::SUPPORT_ANGLE:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    float deg = std::clamp(val, -75.0f, 75.0f);
                    world.bodies[state.selectedBodyId].angle = deg * 0.0174532925f;
                }
                break;
            case EditField::ROPE_LENGTH:
                if (state.selectedRopeId >= 0 && state.selectedRopeId < (int)world.ropes.size()) {
                    world.ropes[state.selectedRopeId].restLength = std::clamp(val, 0.3f, 15.0f);
                    if (state.paused) world.syncRopeLengthToAttachedBody(state.selectedRopeId);
                }
                break;
            case EditField::ROPE_STIFFNESS:
                if (state.selectedRopeId >= 0 && state.selectedRopeId < (int)world.ropes.size()) {
                    world.ropes[state.selectedRopeId].stiffness = std::clamp(val, 10.0f, 2000.0f);
                }
                break;
            case EditField::WORLD_GRAVITY:
                world.G = std::clamp(val, 0.1f, 50.0f);
                break;
            case EditField::WORLD_SUBSTEPS:
                world.substeps = std::clamp((int)std::round(val), 4, 100);
                break;
            case EditField::NEW_ROPE_LENGTH:
                state.newRopeDefaultLength = std::clamp(val, 0.3f, 12.0f);
                break;
            case EditField::BODY_TRAIL_LIFETIME:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    Body& b = world.bodies[state.selectedBodyId];
                    b.trailLifetime = std::clamp(val, 0.2f, 300.0f);
                }
                break;
            case EditField::WORLD_TRAIL_LIFETIME:
                world.defaultTrailLifetime = std::clamp(val, 0.2f, 300.0f);
                for (auto& b : world.bodies) {
                    b.trailLifetime = world.defaultTrailLifetime;
                }
                break;
            default:
                break;
        }
        world.updateStaticTelemetry();
        if (state.paused) recordTelemetry(world, state);
    }
    state.editingField = EditField::NONE;
    state.inputBuffer.clear();
}

static void ActivateInputField(PhysicsWorld& world, AppState& state, EditField field) {
    InputHandler::commitEditField(world, state);
    state.editingField = field;
    state.inputBuffer.clear();
}

bool InputHandler::handleChronoBar(PhysicsWorld& world, AppState& state, int screenW) {
    Vector2 m = GetMousePosition();
    Rectangle bar = getChronoBarRect(screenW);
    if (!PointInRect(m, bar)) return false;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        commitEditField(world, state);
        float x = bar.x + 8.0f;
        float y = bar.y + 6.0f;
        float h = 28.0f;

        // 1. Индикатор времени (104px)
        x += 110.0f;

        // 2. Кнопка сброса времени и трассеров "Сброс t" (68px)
        if (PointInRect(m, {x, y, 68.0f, h})) {
            world.simTime = 0.0f;
            for (auto& b : world.bodies) b.clearTrail();
            clearTelemetry(world, state);
            return true;
        }
        x += 74.0f;

        // 3. Кнопка пошагового кадра ">| Шаг" (68px)
        if (PointInRect(m, {x, y, 68.0f, h})) {
            if (state.paused && world.simTime == 0.0f) world.saveSnapshot();
            state.paused = true;
            state.stepOneFrame = true;
            return true;
        }
        x += 80.0f; // 68px кнопка + 12px отступ с разделителем

        // 4. Переключатели скорости времени: 0.25x, 0.5x, 1.0x
        float scales[3] = {0.25f, 0.5f, 1.0f};
        for (int i = 0; i < 3; ++i) {
            if (PointInRect(m, {x, y, 48.0f, h})) {
                state.timeScale = scales[i];
                return true;
            }
            x += 52.0f;
        }
    }
    return true;
}

bool InputHandler::handleGraphWindow(PhysicsWorld& world, AppState& state, int screenW, int screenH) {
    if (!state.showGraph) {
        state.isDraggingGraph = false;
        state.isResizingGraph = false;
        return false;
    }

    Vector2 m = GetMousePosition();
    Rectangle gr = getGraphRect(state, screenW, screenH);
    float canvasW = (float)(screenW - INSPECTOR_WIDTH);

    // Изменение размера окна осциллографа за правый нижний угол
    if (state.isResizingGraph) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float maxW = std::max(380.0f, canvasW - gr.x - 12.0f);
            float maxH = std::max(190.0f, (float)screenH - gr.y - 12.0f);
            state.graphSize.x = std::clamp(m.x - gr.x, 380.0f, std::min(780.0f, maxW));
            state.graphSize.y = std::clamp(m.y - gr.y, 190.0f, std::min(540.0f, maxH));
            return true;
        } else {
            state.isResizingGraph = false;
        }
    }

    // Перетаскивание окна осциллографа
    if (state.isDraggingGraph) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            state.graphPos.x = std::clamp(m.x - state.graphDragOffset.x, 12.0f, std::max(12.0f, canvasW - gr.width - 12.0f));
            state.graphPos.y = std::clamp(m.y - state.graphDragOffset.y, 12.0f, std::max(12.0f, (float)screenH - gr.height - 12.0f));
            return true;
        } else {
            state.isDraggingGraph = false;
        }
    }

    if (!PointInRect(m, gr)) return false;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        commitEditField(world, state);
        state.graphPos = {gr.x, gr.y};

        if (PointInRect(m, {gr.x + gr.width - 20.0f, gr.y + gr.height - 20.0f, 20.0f, 20.0f})) {
            state.isResizingGraph = true;
            return true;
        }
        if (PointInRect(m, {gr.x + gr.width - 266.0f, gr.y + 6.0f, 90.0f, 24.0f})) {
            state.graphTab = 0;
            return true;
        }
        if (PointInRect(m, {gr.x + gr.width - 170.0f, gr.y + 6.0f, 136.0f, 24.0f})) {
            state.graphTab = 1;
            return true;
        }
        if (PointInRect(m, {gr.x + gr.width - 28.0f, gr.y + 6.0f, 22.0f, 24.0f})) {
            state.showGraph = false;
            state.isDraggingGraph = false;
            state.isResizingGraph = false;
            return true;
        }

        // 5. Захват окна за шапку/рамку для перемещения
        state.isDraggingGraph = true;
        state.graphDragOffset = {m.x - gr.x, m.y - gr.y};
    }
    return true;
}

bool InputHandler::handleDock(PhysicsWorld& world, AppState& state, int screenH) {
    Vector2 m = GetMousePosition();
    state.hoveredDockBtn = -1;
    state.hoveredAddMenuBtn = -1;

    if (state.addMenuOpen) {
        Rectangle menuRect = getAddMenuRect(screenH);
        if (PointInRect(m, menuRect)) {
            for (int i = 0; i < 5; ++i) {
                if (PointInRect(m, getAddMenuItemRect(i, screenH))) {
                    state.hoveredAddMenuBtn = i;
                    break;
                }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && state.hoveredAddMenuBtn != -1) {
                commitEditField(world, state);
                ToolType addTools[5] = {
                    ToolType::ADD_BLOCK,
                    ToolType::ADD_CIRCLE,
                    ToolType::ADD_SUPPORT,
                    ToolType::ADD_ROPE,
                    ToolType::ADD_SPRING
                };
                state.activeTool = addTools[state.hoveredAddMenuBtn];
                state.ropeStartBodyId = -1;
                state.creatingSupportBodyId = -1;
                state.addMenuOpen = false;
            }
            return true;
        }
    }

    Rectangle dock = getDockRect(screenH);
    if (!PointInRect(m, dock)) return false;

    for (int i = 0; i < 7; ++i) {
        if (PointInRect(m, getDockButtonRect(i, screenH))) {
            state.hoveredDockBtn = i;
            break;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && state.hoveredDockBtn != -1) {
        commitEditField(world, state);

        if (state.hoveredDockBtn == 0) {
            if (state.paused && world.simTime == 0.0f) world.saveSnapshot();
            state.paused = !state.paused;
        } else if (state.hoveredDockBtn == 1) {
            state.draggedBodyId = -1;
            state.draggedFreeRopeId = -1;
            state.creatingSupportBodyId = -1;
            state.paused = true;
            world.restoreSnapshot();
            clearTelemetry(world, state);
        } else if (state.hoveredDockBtn == 2) {
            state.activeTool = ToolType::SELECT_MOVE;
            state.ropeStartBodyId = -1;
            state.creatingSupportBodyId = -1;
            state.addMenuOpen = false;
        } else if (state.hoveredDockBtn == 3) {
            state.addMenuOpen = !state.addMenuOpen;
        } else if (state.hoveredDockBtn == 4) {
            // [3] Осциллограф
            state.showGraph = !state.showGraph;
            state.addMenuOpen = false;
        } else if (state.hoveredDockBtn == 5) {
            // [4] Глазик: Скрыть / Показать все тела (оставив трассеры)
            state.hideAllBodies = !state.hideAllBodies;
            state.addMenuOpen = false;
        } else if (state.hoveredDockBtn == 6) {
            // [5] Удалить (мусорка)
            state.activeTool = ToolType::DELETE_OBJ;
            state.ropeStartBodyId = -1;
            state.creatingSupportBodyId = -1;
            state.addMenuOpen = false;
        }
    }
    return true;
}

void InputHandler::handleInspector(PhysicsWorld& world, AppState& state, int screenW, int screenH) {
    Vector2 m = GetMousePosition();
    float panelX = (float)(screenW - INSPECTOR_WIDTH);
    float topY = 12.0f;

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || m.x < panelX) return;

    float x = panelX + 16.0f;
    float w = INSPECTOR_WIDTH - 32.0f;

    if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
        Body& b = world.bodies[state.selectedBodyId];

        if (b.isStatic) {
            if (PointInRect(m, {x, topY + 44, w, 30})) {
                commitEditField(world, state);
                b.supportLocked = !b.supportLocked;
                return;
            }
            if (CheckInputBoxClick(m, x, topY + 84))  { ActivateInputField(world, state, EditField::SUPPORT_X); return; }
            if (CheckInputBoxClick(m, x, topY + 138)) { ActivateInputField(world, state, EditField::SUPPORT_Y); return; }
            if (CheckInputBoxClick(m, x, topY + 192)) { ActivateInputField(world, state, EditField::SUPPORT_WIDTH); return; }
            if (CheckInputBoxClick(m, x, topY + 246)) { ActivateInputField(world, state, EditField::SUPPORT_ANGLE); return; }

            if (PointInRect(m, {x, topY + 302, w, 26})) {
                commitEditField(world, state);
                b.angle = 0.0f;
                return;
            }
            if (PointInRect(m, {x, (float)screenH - 48.0f, w, 32})) {
                commitEditField(world, state);
                int delId = state.selectedBodyId;
                state.selectedBodyId = -1;
                state.creatingSupportBodyId = -1;
                world.removeBody(delId);
                clearTelemetry(world, state);
                return;
            }
            commitEditField(world, state);
            return;
        } else {
            if (CheckInputBoxClick(m, x, topY + 40))  { ActivateInputField(world, state, EditField::BODY_MASS); return; }
            if (CheckInputBoxClick(m, x, topY + 90))  { ActivateInputField(world, state, EditField::BODY_FRICTION); return; }

            if (b.shape == ShapeType::CIRCLE) {
                if (CheckInputBoxClick(m, x, topY + 140)) { ActivateInputField(world, state, EditField::BODY_RADIUS); return; }
                if (CheckInputBoxClick(m, x, topY + 190)) { ActivateInputField(world, state, EditField::BODY_RESTITUTION); return; }
            } else {
                if (CheckInputBoxClick(m, x, topY + 140)) { ActivateInputField(world, state, EditField::BODY_WIDTH); return; }
                if (CheckInputBoxClick(m, x, topY + 190)) { ActivateInputField(world, state, EditField::BODY_HEIGHT); return; }
            }

            if (CheckInputBoxClick(m, x, topY + 240)) { ActivateInputField(world, state, EditField::BODY_FORCE_X); return; }

            if (PointInRect(m, {x, topY + 294, 130, 26})) {
                commitEditField(world, state);
                b.canRotate = !b.canRotate;
                if (!b.canRotate) { b.angle = 0.0f; b.angVel = 0.0f; }
                b.updateMassData();
                return;
            }
            if (PointInRect(m, {x + 138, topY + 294, 130, 26})) {
                commitEditField(world, state);
                b.angle = 0.0f;
                b.angVel = 0.0f;
                world.resolveEditorOverlap(b.id);
                return;
            }

            if (PointInRect(m, {x, topY + 326, w, 26})) {
                commitEditField(world, state);
                b.hidden = !b.hidden;
                return;
            }

            // 3 отдельные кнопки трассера: [ Нет ] [ По времени ] [ Вечный ]
            float trBtnY = topY + 376.0f;
            if (PointInRect(m, {x, trBtnY, 76.0f, 26.0f})) {
                commitEditField(world, state);
                b.trailMode = 0;
                b.clearTrail();
                return;
            }
            if (PointInRect(m, {x + 82.0f, trBtnY, 104.0f, 26.0f})) {
                commitEditField(world, state);
                b.trailMode = 1;
                return;
            }
            if (PointInRect(m, {x + 192.0f, trBtnY, 76.0f, 26.0f})) {
                commitEditField(world, state);
                b.trailMode = 2;
                return;
            }

            if (b.trailMode == 1 && CheckInputBoxClick(m, x, topY + 408.0f)) {
                ActivateInputField(world, state, EditField::BODY_TRAIL_LIFETIME);
                return;
            }

            if (PointInRect(m, {x, (float)screenH - 48.0f, w, 32})) {
                commitEditField(world, state);
                int delId = state.selectedBodyId;
                state.selectedBodyId = -1;
                world.removeBody(delId);
                clearTelemetry(world, state);
                return;
            }
            commitEditField(world, state);
            return;
        }
    }

    if (state.selectedRopeId >= 0 && state.selectedRopeId < (int)world.ropes.size()) {
        RopeLink& r = world.ropes[state.selectedRopeId];

        if (PointInRect(m, {x, topY + 44, w, 30})) {
            commitEditField(world, state);
            r.isRigid = !r.isRigid;
            world.updateStaticTelemetry();
            return;
        }
        if (CheckInputBoxClick(m, x, topY + 84)) { ActivateInputField(world, state, EditField::ROPE_LENGTH); return; }
        if (PointInRect(m, {x, topY + 140, w, 28})) {
            commitEditField(world, state);
            world.syncRopeLengthToAttachedBody(r.id);
            return;
        }
        if (!r.isRigid && CheckInputBoxClick(m, x, topY + 178)) {
            ActivateInputField(world, state, EditField::ROPE_STIFFNESS);
            return;
        }
        if (r.idB != -1 && PointInRect(m, {x, topY + 236, w, 28})) {
            commitEditField(world, state);
            r.freePos = world.bodies[r.idB].localToWorld(r.offsetB);
            r.freeVel = {0.0f, 0.0f};
            r.idB = -1;
            world.updateStaticTelemetry();
            return;
        }
        if (PointInRect(m, {x, (float)screenH - 48.0f, w, 32})) {
            commitEditField(world, state);
            int delId = state.selectedRopeId;
            state.selectedRopeId = -1;
            world.removeRope(delId);
            return;
        }
        commitEditField(world, state);
        return;
    }

    // Параметры среды (когда ничего не выбрано)
    if (CheckInputBoxClick(m, x, topY + 40)) { ActivateInputField(world, state, EditField::WORLD_GRAVITY); return; }
    if (PointInRect(m, {x, topY + 92, 128, 26}))       { commitEditField(world, state); world.G = 9.81f; world.updateStaticTelemetry(); return; }
    if (PointInRect(m, {x + 140, topY + 92, 128, 26})) { commitEditField(world, state); world.G = 10.0f; world.updateStaticTelemetry(); return; }
    if (CheckInputBoxClick(m, x, topY + 124)) { ActivateInputField(world, state, EditField::WORLD_SUBSTEPS); return; }
    if (CheckInputBoxClick(m, x, topY + 174)) { ActivateInputField(world, state, EditField::NEW_ROPE_LENGTH); return; }

    if (PointInRect(m, {x, topY + 226, w, 28})) {
        commitEditField(world, state);
        state.camPos = {-0.8f, 4.3f};
        state.camZoom = 1.0f;
        return;
    }

    // 3 отдельные кнопки глобального режима трассеров: [ Нет ] [ По времени ] [ Вечный ]
    float wTrBtnY = topY + 280.0f;
    if (PointInRect(m, {x, wTrBtnY, 76.0f, 26.0f})) {
        commitEditField(world, state);
        world.defaultTrailMode = 0;
        for (auto& b : world.bodies) { b.trailMode = 0; b.clearTrail(); }
        return;
    }
    if (PointInRect(m, {x + 82.0f, wTrBtnY, 104.0f, 26.0f})) {
        commitEditField(world, state);
        world.defaultTrailMode = 1;
        for (auto& b : world.bodies) b.trailMode = 1;
        return;
    }
    if (PointInRect(m, {x + 192.0f, wTrBtnY, 76.0f, 26.0f})) {
        commitEditField(world, state);
        world.defaultTrailMode = 2;
        for (auto& b : world.bodies) b.trailMode = 2;
        return;
    }

    float sepY = topY + 314.0f;
    if (world.defaultTrailMode == 1) {
        if (CheckInputBoxClick(m, x, topY + 312.0f)) {
            ActivateInputField(world, state, EditField::WORLD_TRAIL_LIFETIME);
            return;
        }
        sepY = topY + 368.0f;
    }

    float labY = sepY + 30.0f;
    for (int i = 0; i < 4; ++i) {
        if (PointInRect(m, {x, labY + i * 34.0f, w, 29.0f})) {
            commitEditField(world, state);
            world.loadLabPreset(i + 1);
            state.paused = true;
            state.selectedBodyId = -1;
            state.selectedRopeId = -1;
            state.camPos = {0.0f, 4.3f};
            state.camZoom = 1.0f;
            if (i == 3) state.forceViewMode = 1;
            clearTelemetry(world, state);
            return;
        }
    }

    if (PointInRect(m, {x, labY + 4 * 34.0f + 6.0f, 130.0f, 28.0f})) {
        commitEditField(world, state);
        world.setupDefaultScene();
        state.paused = true;
        state.camPos = {-0.8f, 4.3f};
        state.camZoom = 1.0f;
        clearTelemetry(world, state);
        return;
    }
    if (PointInRect(m, {x + 138.0f, labY + 4 * 34.0f + 6.0f, 130.0f, 28.0f})) {
        commitEditField(world, state);
        world.clearScene();
        state.paused = true;
        clearTelemetry(world, state);
        return;
    }

    commitEditField(world, state);
}

void InputHandler::handleKeyboardInput(PhysicsWorld& world, AppState& state) {
    if (state.editingField != EditField::NONE) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= '0' && key <= '9') || key == '.' || key == ',' || key == '-') {
                if (key == ',') key = '.';
                if (state.inputBuffer.size() < 8) state.inputBuffer += (char)key;
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !state.inputBuffer.empty()) {
            state.inputBuffer.pop_back();
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            commitEditField(world, state);
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            state.editingField = EditField::NONE;
            state.inputBuffer.clear();
        }
        return;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        if (state.paused && world.simTime == 0.0f) world.saveSnapshot();
        state.paused = !state.paused;
    }
    if (IsKeyPressed(KEY_V)) {
        state.forceViewMode = (state.forceViewMode + 1) % 3;
    }
    if (IsKeyPressed(KEY_R)) {
        state.draggedBodyId = -1;
        state.draggedFreeRopeId = -1;
        state.creatingSupportBodyId = -1;
        state.paused = true;
        world.restoreSnapshot();
        clearTelemetry(world, state);
    }
    if (IsKeyPressed(KEY_H)) {
        state.hideAllBodies = !state.hideAllBodies;
    }
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        if (state.selectedBodyId >= 0) {
            int id = state.selectedBodyId;
            state.selectedBodyId = -1;
            state.creatingSupportBodyId = -1;
            world.removeBody(id);
            clearTelemetry(world, state);
        } else if (state.selectedRopeId >= 0) {
            int id = state.selectedRopeId;
            state.selectedRopeId = -1;
            world.removeRope(id);
        }
    }

    if (state.addMenuOpen) {
        if (IsKeyPressed(KEY_ONE)) {
            state.activeTool = ToolType::ADD_BLOCK;
            state.ropeStartBodyId = -1;
            state.addMenuOpen = false;
        } else if (IsKeyPressed(KEY_TWO)) {
            state.activeTool = ToolType::ADD_CIRCLE;
            state.ropeStartBodyId = -1;
            state.addMenuOpen = false;
        } else if (IsKeyPressed(KEY_THREE)) {
            state.activeTool = ToolType::ADD_SUPPORT;
            state.ropeStartBodyId = -1;
            state.addMenuOpen = false;
        } else if (IsKeyPressed(KEY_FOUR)) {
            state.activeTool = ToolType::ADD_ROPE;
            state.ropeStartBodyId = -1;
            state.addMenuOpen = false;
        } else if (IsKeyPressed(KEY_FIVE)) {
            state.activeTool = ToolType::ADD_SPRING;
            state.ropeStartBodyId = -1;
            state.addMenuOpen = false;
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            state.addMenuOpen = false;
        }
    } else {
        // Шорткаты нижнего дока: [1] Курсор, [2] Создать, [3] Осциллограф, [4] Глазик, [5] Удалить
        if (IsKeyPressed(KEY_ONE)) {
            state.activeTool = ToolType::SELECT_MOVE;
            state.ropeStartBodyId = -1;
        } else if (IsKeyPressed(KEY_TWO)) {
            state.addMenuOpen = true;
        } else if (IsKeyPressed(KEY_THREE)) {
            state.showGraph = !state.showGraph;
        } else if (IsKeyPressed(KEY_FOUR)) {
            state.hideAllBodies = !state.hideAllBodies;
        } else if (IsKeyPressed(KEY_FIVE)) {
            state.activeTool = ToolType::DELETE_OBJ;
            state.ropeStartBodyId = -1;
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            state.ropeStartBodyId = -1;
            state.creatingSupportBodyId = -1;
            state.activeTool = ToolType::SELECT_MOVE;
            state.selectedBodyId = -1;
            state.selectedRopeId = -1;
        }
    }
}

void InputHandler::update(PhysicsWorld& world, AppState& state, int screenW, int screenH, float /*ppm*/) {
    float canvasWidth = (float)(screenW - INSPECTOR_WIDTH);
    float centerX = canvasWidth * 0.5f;
    float centerY = screenH * 0.5f;
    float effPpm = BASE_PPM * state.camZoom;

    bool overGraph  = handleGraphWindow(world, state, screenW, screenH);
    bool overChrono = !overGraph && handleChronoBar(world, state, screenW);
    bool overDock   = !overGraph && handleDock(world, state, screenH);
    handleInspector(world, state, screenW, screenH);
    handleKeyboardInput(world, state);

    Vector2 mouse = GetMousePosition();
    bool insideCanvas = (!overChrono && !overGraph && !overDock &&
                         mouse.x >= 0 && mouse.x < canvasWidth && mouse.y >= 0 && mouse.y <= screenH);

    if (insideCanvas && (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))) {
        state.isPanning = true;
        state.panStartMouse = mouse;
        state.panStartCam = state.camPos;
    }
    if (state.isPanning) {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            float dx = (mouse.x - state.panStartMouse.x) / effPpm;
            float dy = (mouse.y - state.panStartMouse.y) / effPpm;
            state.camPos = {state.panStartCam.x - dx, state.panStartCam.y + dy};
        } else {
            state.isPanning = false;
        }
    }

    state.mouseWorldPos = {
        state.camPos.x + (mouse.x - centerX) / effPpm,
        state.camPos.y + (centerY - mouse.y) / effPpm
    };

    state.hoveredBodyId = -1;
    state.hoveredRopeId = -1;

    if (insideCanvas) {
        for (const auto& b : world.bodies) {
            if (b.containsPoint(state.mouseWorldPos)) {
                state.hoveredBodyId = b.id;
                break;
            }
        }

        if (state.hoveredBodyId == -1 && state.draggedBodyId == -1) {
            const float ROPE_RADIUS = 0.25f;
            for (const auto& r : world.ropes) {
                Vec2 pA = world.bodies[r.idA].localToWorld(r.offsetA);
                Vec2 pB = world.getRopeEndWorldPos(r);
                if (Vec2::distToSegment(state.mouseWorldPos, pA, pB) <= ROPE_RADIUS) {
                    state.hoveredRopeId = r.id;
                    break;
                }
            }
        }

        float wheel = GetMouseWheelMove();
        if (std::abs(wheel) > 1e-3f && state.creatingSupportBodyId == -1) {
            bool shiftHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            int rotId = (state.draggedBodyId != -1) ? state.draggedBodyId
                      : ((state.hoveredBodyId != -1) ? state.hoveredBodyId : state.selectedBodyId);

            if (shiftHeld && rotId >= 0 && rotId < (int)world.bodies.size()) {
                Body& rb = world.bodies[rotId];
                rb.angle += wheel * 5.0f * 0.0174532925f;
                if (rb.isStatic) {
                    rb.angle = std::clamp(rb.angle, -75.0f * 0.0174532925f, 75.0f * 0.0174532925f);
                }
                rb.angVel = 0.0f;
                world.resolveEditorOverlap(rb.id);
                if (state.paused) world.saveSnapshot();
            } else {
                Vec2 worldBefore = state.mouseWorldPos;
                float zoomFactor = (wheel > 0.0f) ? 1.12f : (1.0f / 1.12f);
                state.camZoom = std::clamp(state.camZoom * zoomFactor, 0.20f, 3.50f);

                float newEffPpm = BASE_PPM * state.camZoom;
                Vec2 worldAfter = {
                    state.camPos.x + (mouse.x - centerX) / newEffPpm,
                    state.camPos.y + (centerY - mouse.y) / newEffPpm
                };
                state.camPos += (worldBefore - worldAfter);
                state.mouseWorldPos = worldBefore;
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            commitEditField(world, state);
            state.addMenuOpen = false;

            switch (state.activeTool) {
                case ToolType::SELECT_MOVE: {
                    for (const auto& r : world.ropes) {
                        if (r.idB == -1 && Vec2::dist(state.mouseWorldPos, r.freePos) <= 0.30f) {
                            state.selectedRopeId = r.id;
                            state.selectedBodyId = -1;
                            state.draggedFreeRopeId = r.id;
                            return;
                        }
                    }

                    if (state.hoveredBodyId != -1) {
                        state.selectedBodyId = state.hoveredBodyId;
                        state.selectedRopeId = -1;
                        const Body& hb = world.bodies[state.hoveredBodyId];
                        if (!hb.isStatic) {
                            recordTelemetry(world, state);
                        }
                        if (!hb.isStatic || !hb.supportLocked) {
                            state.draggedBodyId = state.hoveredBodyId;
                            state.dragOffset = hb.pos - state.mouseWorldPos;
                        }
                    } else if (state.hoveredRopeId != -1) {
                        state.selectedRopeId = state.hoveredRopeId;
                        state.selectedBodyId = -1;
                    } else {
                        state.selectedBodyId = -1;
                        state.selectedRopeId = -1;
                    }
                    break;
                }
                case ToolType::ADD_BLOCK: {
                    Vec2 safePos = world.findSafeSpawnPos(state.mouseWorldPos, 1.0f, 1.0f);
                    int id = world.createBody(safePos, 1.0f, 1.0f, 2.0f, false);
                    world.tryAttachBodyToFreeRopes(id);
                    state.selectedBodyId = id;
                    state.selectedRopeId = -1;
                    world.updateStaticTelemetry();
                    world.saveSnapshot();
                    recordTelemetry(world, state);
                    break;
                }
                case ToolType::ADD_CIRCLE: {
                    Vec2 safePos = world.findSafeSpawnPos(state.mouseWorldPos, 1.0f, 1.0f);
                    int id = world.createCircle(safePos, 0.5f, 1.5f, false);
                    world.tryAttachBodyToFreeRopes(id);
                    state.selectedBodyId = id;
                    state.selectedRopeId = -1;
                    world.updateStaticTelemetry();
                    world.saveSnapshot();
                    recordTelemetry(world, state);
                    break;
                }
                case ToolType::ADD_SUPPORT: {
                    state.supportStartPos = state.mouseWorldPos;
                    int id = world.createBody(state.mouseWorldPos, 0.5f, 0.24f, 1.0f, true);
                    state.creatingSupportBodyId = id;
                    state.selectedBodyId = id;
                    state.selectedRopeId = -1;
                    world.updateStaticTelemetry();
                    break;
                }
                case ToolType::ADD_ROPE:
                case ToolType::ADD_SPRING: {
                    bool isSpring = (state.activeTool == ToolType::ADD_SPRING);
                    if (state.ropeStartBodyId == -1) {
                        if (state.hoveredBodyId != -1) {
                            state.ropeStartBodyId = state.hoveredBodyId;
                            state.ropeStartOffset = world.getSnapOffset(state.hoveredBodyId, state.mouseWorldPos);
                        }
                    } else {
                        Vec2 pA = world.bodies[state.ropeStartBodyId].localToWorld(state.ropeStartOffset);
                        if (state.hoveredBodyId != -1 && state.hoveredBodyId != state.ropeStartBodyId) {
                            Vec2 snapB = world.getSnapOffset(state.hoveredBodyId, state.mouseWorldPos);
                            Body& bodyB = world.bodies[state.hoveredBodyId];
                            Vec2 pB = bodyB.localToWorld(snapB);
                            float dist = (pB - pA).length();

                            float ropeLen = std::clamp(dist, 0.4f, 12.0f);
                            if (!isSpring && !bodyB.isStatic && bodyB.pos.y - bodyB.height * 0.5f <= world.groundY + 0.1f && dist > state.newRopeDefaultLength) {
                                ropeLen = state.newRopeDefaultLength;
                            }

                            int rId = world.addRope(state.ropeStartBodyId, state.hoveredBodyId,
                                                    state.ropeStartOffset, snapB, ropeLen, 180.0f, false, isSpring);
                            if (!isSpring) world.syncRopeLengthToAttachedBody(rId);
                            else world.updateStaticTelemetry();
                            state.selectedRopeId = rId;
                            state.selectedBodyId = -1;
                            state.ropeStartBodyId = -1;
                            world.saveSnapshot();
                        } else {
                            float dragDist = (state.mouseWorldPos - pA).length();
                            float len = (dragDist > 0.4f) ? std::clamp(dragDist, 0.4f, 12.0f) : state.newRopeDefaultLength;
                            int rId = world.addFreeRope(state.ropeStartBodyId, state.ropeStartOffset, state.mouseWorldPos, len, 180.0f, false, isSpring);
                            state.selectedRopeId = rId;
                            state.selectedBodyId = -1;
                            state.ropeStartBodyId = -1;
                            world.updateStaticTelemetry();
                            world.saveSnapshot();
                        }
                    }
                    break;
                }
                case ToolType::DELETE_OBJ: {
                    if (state.hoveredBodyId != -1) {
                        if (state.selectedBodyId == state.hoveredBodyId) state.selectedBodyId = -1;
                        world.removeBody(state.hoveredBodyId);
                        world.saveSnapshot();
                        clearTelemetry(world, state);
                    } else if (state.hoveredRopeId != -1) {
                        if (state.selectedRopeId == state.hoveredRopeId) state.selectedRopeId = -1;
                        world.removeRope(state.hoveredRopeId);
                        world.saveSnapshot();
                    }
                    break;
                }
            }
        }
    }

    if (state.creatingSupportBodyId >= 0 && state.creatingSupportBodyId < (int)world.bodies.size()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Body& sb = world.bodies[state.creatingSupportBodyId];
            Vec2 start = state.supportStartPos;
            Vec2 end = state.mouseWorldPos;
            Vec2 delta = end - start;
            float dist = delta.length();

            if (dist >= 0.15f) {
                if (std::abs(delta.y) < 0.14f && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
                    end.y = start.y;
                    delta = end - start;
                    dist = std::abs(delta.x);
                    sb.angle = 0.0f;
                } else {
                    Vec2 dir = (delta.x >= 0.0f) ? delta : (delta * -1.0f);
                    float ang = std::atan2(dir.y, dir.x);
                    sb.angle = std::clamp(ang, -75.0f * 0.0174532925f, 75.0f * 0.0174532925f);
                }
                sb.pos = (start + end) * 0.5f;
                sb.width = std::clamp(dist, 0.5f, 30.0f);
            } else {
                sb.pos = start;
                sb.width = 0.5f;
                sb.angle = 0.0f;
            }
            world.resolveEditorOverlap(sb.id);
        }
    }

    if (state.draggedBodyId >= 0 && state.draggedBodyId < (int)world.bodies.size()) {
        Body& db = world.bodies[state.draggedBodyId];
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (state.paused || (db.isStatic && !db.supportLocked))) {
            db.pos = state.mouseWorldPos + state.dragOffset;
            db.vel = {0.0f, 0.0f};
            db.angVel = 0.0f;
            db.clearTrail();
            world.resolveEditorOverlap(db.id);
            world.tryAttachBodyToFreeRopes(db.id);
            if (state.paused) recordTelemetry(world, state);
        }
    }

    if (state.paused && state.draggedFreeRopeId >= 0 && state.draggedFreeRopeId < (int)world.ropes.size()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            RopeLink& r = world.ropes[state.draggedFreeRopeId];
            Vec2 pA = world.bodies[r.idA].localToWorld(r.offsetA);
            Vec2 d = state.mouseWorldPos - pA;
            r.restLength = std::clamp(d.length(), 0.3f, 12.0f);
            r.currentLength = r.restLength;
            Vec2 n = (d.length() > 1e-4f) ? d.normalized() : Vec2{0.0f, -1.0f};
            r.freePos = pA + n * r.restLength;
            world.updateStaticTelemetry();
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (state.creatingSupportBodyId >= 0 && state.creatingSupportBodyId < (int)world.bodies.size()) {
            Body& sb = world.bodies[state.creatingSupportBodyId];
            if ((state.mouseWorldPos - state.supportStartPos).length() < 0.15f) {
                sb.width = 3.0f;
                sb.angle = 0.0f;
            }
            world.resolveEditorOverlap(sb.id);
            if (state.paused) world.saveSnapshot();
        } else if (state.draggedBodyId != -1 || state.draggedFreeRopeId != -1) {
            if (state.paused && world.simTime == 0.0f) world.saveSnapshot();
        }
        state.creatingSupportBodyId = -1;
        state.draggedBodyId = -1;
        state.draggedFreeRopeId = -1;
    }
}