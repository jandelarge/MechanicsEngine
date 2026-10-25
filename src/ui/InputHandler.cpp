#include "InputHandler.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

static bool PointInRect(Vector2 p, Rectangle r) {
    return (p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height);
}

static bool CheckInputBoxClick(Vector2 m, float x, float y) {
    float by = y + 20.0f;
    return PointInRect(m, {x, by, 268.0f, 28.0f});
}

Rectangle InputHandler::getDockRect(int screenH) {
    // 5 кнопок: Пуск, Сброс | Курсор [1], Добавить [2], Удалить [3]
    return {20.0f, (float)screenH - 78.0f, 282.0f, 58.0f};
}

Rectangle InputHandler::getDockButtonRect(int index, int screenH) {
    Rectangle dock = getDockRect(screenH);
    float btnSize = 44.0f;
    float y = dock.y + (dock.height - btnSize) * 0.5f;
    float x = dock.x + 8.0f;

    for (int i = 0; i < index; ++i) {
        x += btnSize + 6.0f;
        if (i == 1) x += 12.0f; // Разделитель после кнопок Пуск и Сброс
    }
    return {x, y, btnSize, btnSize};
}

Rectangle InputHandler::getAddMenuRect(int screenH) {
    Rectangle dock = getDockRect(screenH);
    // Всплывающая панель ровно над доком
    return {dock.x, dock.y - 70.0f, 386.0f, 62.0f};
}

Rectangle InputHandler::getAddMenuItemRect(int index, int screenH) {
    Rectangle menu = getAddMenuRect(screenH);
    float itemW = 88.0f;
    float itemH = 48.0f;
    float x = menu.x + 8.0f + index * (itemW + 6.0f);
    float y = menu.y + (menu.height - itemH) * 0.5f;
    return {x, y, itemW, itemH};
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
                    world.bodies[state.selectedBodyId].width = std::clamp(val, 0.4f, 6.0f);
                    world.bodies[state.selectedBodyId].updateMassData();
                }
                break;
            case EditField::BODY_HEIGHT:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].height = std::clamp(val, 0.4f, 6.0f);
                    world.bodies[state.selectedBodyId].updateMassData();
                }
                break;
            case EditField::BODY_RADIUS:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].setRadius(val);
                    world.resolveEditorOverlap(state.selectedBodyId);
                }
                break;
            case EditField::SUPPORT_X:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].pos.x = std::clamp(val, -7.5f, 7.5f);
                }
                break;
            case EditField::SUPPORT_Y:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].pos.y = std::clamp(val, 0.6f, 8.5f);
                }
                break;
            case EditField::SUPPORT_WIDTH:
                if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
                    world.bodies[state.selectedBodyId].width = std::clamp(val, 0.5f, 8.0f);
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
                    world.ropes[state.selectedRopeId].restLength = std::clamp(val, 0.3f, 8.0f);
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
                state.newRopeDefaultLength = std::clamp(val, 0.3f, 7.0f);
                break;
            default:
                break;
        }
        world.updateStaticTelemetry();
    }
    state.editingField = EditField::NONE;
    state.inputBuffer.clear();
}

static void ActivateInputField(PhysicsWorld& world, AppState& state, EditField field) {
    InputHandler::commitEditField(world, state);
    state.editingField = field;
    state.inputBuffer.clear();
}

bool InputHandler::handleDock(PhysicsWorld& world, AppState& state, int screenH) {
    Vector2 m = GetMousePosition();
    state.hoveredDockBtn = -1;
    state.hoveredAddMenuBtn = -1;

    // 1. Проверяем клик и наведение по открытому меню [+]
    if (state.addMenuOpen) {
        Rectangle menuRect = getAddMenuRect(screenH);
        if (PointInRect(m, menuRect)) {
            for (int i = 0; i < 4; ++i) {
                if (PointInRect(m, getAddMenuItemRect(i, screenH))) {
                    state.hoveredAddMenuBtn = i;
                    break;
                }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && state.hoveredAddMenuBtn != -1) {
                commitEditField(world, state);
                ToolType addTools[4] = {
                    ToolType::ADD_BLOCK,
                    ToolType::ADD_CIRCLE,
                    ToolType::ADD_SUPPORT,
                    ToolType::ADD_ROPE
                };
                state.activeTool = addTools[state.hoveredAddMenuBtn];
                state.ropeStartBodyId = -1;
                state.addMenuOpen = false;
            }
            return true;
        }
    }

    // 2. Проверяем основной док (5 кнопок)
    Rectangle dock = getDockRect(screenH);
    if (!PointInRect(m, dock)) return false;

    for (int i = 0; i < 5; ++i) {
        if (PointInRect(m, getDockButtonRect(i, screenH))) {
            state.hoveredDockBtn = i;
            break;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && state.hoveredDockBtn != -1) {
        commitEditField(world, state);

        if (state.hoveredDockBtn == 0) {
            if (state.paused) world.saveSnapshot();
            state.paused = !state.paused;
        } else if (state.hoveredDockBtn == 1) {
            state.draggedBodyId = -1;
            state.draggedFreeRopeId = -1;
            state.paused = true;
            world.restoreSnapshot();
        } else if (state.hoveredDockBtn == 2) {
            // Кнопка [1]: Курсор
            state.activeTool = ToolType::SELECT_MOVE;
            state.ropeStartBodyId = -1;
            state.addMenuOpen = false;
        } else if (state.hoveredDockBtn == 3) {
            // Кнопка [2]: Меню [+]
            state.addMenuOpen = !state.addMenuOpen;
        } else if (state.hoveredDockBtn == 4) {
            // Кнопка [3]: Удалить
            state.activeTool = ToolType::DELETE_OBJ;
            state.ropeStartBodyId = -1;
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
                world.removeBody(delId);
                return;
            }
            commitEditField(world, state);
            return;
        } else if (b.shape == ShapeType::CIRCLE) {
            if (CheckInputBoxClick(m, x, topY + 44))  { ActivateInputField(world, state, EditField::BODY_MASS); return; }
            if (CheckInputBoxClick(m, x, topY + 98))  { ActivateInputField(world, state, EditField::BODY_FRICTION); return; }
            if (CheckInputBoxClick(m, x, topY + 152)) { ActivateInputField(world, state, EditField::BODY_RADIUS); return; }
            if (CheckInputBoxClick(m, x, topY + 206)) { ActivateInputField(world, state, EditField::BODY_RESTITUTION); return; }

            if (PointInRect(m, {x, topY + 262, w, 28})) {
                commitEditField(world, state);
                b.canRotate = !b.canRotate;
                if (!b.canRotate) { b.angle = 0.0f; b.angVel = 0.0f; }
                b.updateMassData();
                return;
            }

            if (PointInRect(m, {x, topY + 296, w, 26})) {
                commitEditField(world, state);
                b.angle = 0.0f;
                b.angVel = 0.0f;
                return;
            }

            if (PointInRect(m, {x, (float)screenH - 48.0f, w, 32})) {
                commitEditField(world, state);
                int delId = state.selectedBodyId;
                state.selectedBodyId = -1;
                world.removeBody(delId);
                return;
            }
            commitEditField(world, state);
            return;
        } else {
            if (CheckInputBoxClick(m, x, topY + 44))  { ActivateInputField(world, state, EditField::BODY_MASS); return; }
            if (CheckInputBoxClick(m, x, topY + 98))  { ActivateInputField(world, state, EditField::BODY_FRICTION); return; }
            if (CheckInputBoxClick(m, x, topY + 152)) { ActivateInputField(world, state, EditField::BODY_WIDTH); return; }
            if (CheckInputBoxClick(m, x, topY + 206)) { ActivateInputField(world, state, EditField::BODY_HEIGHT); return; }

            if (PointInRect(m, {x, topY + 262, w, 28})) {
                commitEditField(world, state);
                b.canRotate = !b.canRotate;
                if (!b.canRotate) { b.angle = 0.0f; b.angVel = 0.0f; }
                b.updateMassData();
                return;
            }

            if (PointInRect(m, {x, topY + 296, w, 26})) {
                commitEditField(world, state);
                b.angle = 0.0f;
                b.angVel = 0.0f;
                world.resolveEditorOverlap(b.id);
                return;
            }

            if (PointInRect(m, {x, (float)screenH - 48.0f, w, 32})) {
                commitEditField(world, state);
                int delId = state.selectedBodyId;
                state.selectedBodyId = -1;
                world.removeBody(delId);
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

    if (CheckInputBoxClick(m, x, topY + 44)) { ActivateInputField(world, state, EditField::WORLD_GRAVITY); return; }

    if (PointInRect(m, {x, topY + 98, 128, 26}))       { commitEditField(world, state); world.G = 9.81f; world.updateStaticTelemetry(); return; }
    if (PointInRect(m, {x + 140, topY + 98, 128, 26})) { commitEditField(world, state); world.G = 10.0f; world.updateStaticTelemetry(); return; }

    if (CheckInputBoxClick(m, x, topY + 134)) { ActivateInputField(world, state, EditField::WORLD_SUBSTEPS); return; }
    if (CheckInputBoxClick(m, x, topY + 188)) { ActivateInputField(world, state, EditField::NEW_ROPE_LENGTH); return; }

    if (PointInRect(m, {x, topY + 250, w, 30})) {
        commitEditField(world, state);
        world.setupDefaultScene();
        state.paused = true;
        return;
    }
    if (PointInRect(m, {x, topY + 288, w, 30})) {
        commitEditField(world, state);
        world.clearScene();
        state.paused = true;
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
        if (state.paused) world.saveSnapshot();
        state.paused = !state.paused;
    }
    if (IsKeyPressed(KEY_V)) {
        state.forceViewMode = (state.forceViewMode + 1) % 3;
    }
    if (IsKeyPressed(KEY_R)) {
        state.draggedBodyId = -1;
        state.draggedFreeRopeId = -1;
        state.paused = true;
        world.restoreSnapshot();
    }
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        if (state.selectedBodyId >= 0) {
            int id = state.selectedBodyId;
            state.selectedBodyId = -1;
            world.removeBody(id);
        } else if (state.selectedRopeId >= 0) {
            int id = state.selectedRopeId;
            state.selectedRopeId = -1;
            world.removeRope(id);
        }
    }

    // Шорткаты: [1] Курсор, [2] -> [1..4] Добавить, [3] Удалить
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
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            state.addMenuOpen = false; // Закрываем меню выбора на Escape
        }
    } else {
        if (IsKeyPressed(KEY_ONE)) {
            state.activeTool = ToolType::SELECT_MOVE;
            state.ropeStartBodyId = -1;
        } else if (IsKeyPressed(KEY_TWO)) {
            state.addMenuOpen = true;
        } else if (IsKeyPressed(KEY_THREE)) {
            state.activeTool = ToolType::DELETE_OBJ;
            state.ropeStartBodyId = -1;
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            // Если меню закрыто, Escape отменяет протяжку нити и возвращает курсор
            state.ropeStartBodyId = -1;
            state.activeTool = ToolType::SELECT_MOVE;
        }
    }
}

void InputHandler::update(PhysicsWorld& world, AppState& state, int screenW, int screenH, float ppm) {
    float canvasWidth = (float)(screenW - INSPECTOR_WIDTH);
    float originX = canvasWidth * 0.5f;
    float originY = screenH * 0.84f;

    world.leftWallX  = -originX / ppm + 0.08f;
    world.rightWallX = (canvasWidth - originX) / ppm - 0.08f;
    world.ceilingY   = originY / ppm - 0.08f;

    bool overDock = handleDock(world, state, screenH);
    handleInspector(world, state, screenW, screenH);
    handleKeyboardInput(world, state);

    Vector2 mouse = GetMousePosition();
    bool insideCanvas = (!overDock && mouse.x >= 0 && mouse.x < canvasWidth && mouse.y >= 0 && mouse.y <= screenH);

    state.mouseWorldPos = {
        (mouse.x - originX) / ppm,
        (originY - mouse.y) / ppm
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
                    break;
                }
                case ToolType::ADD_SUPPORT: {
                    int id = world.createBody(state.mouseWorldPos, 3.0f, 0.2f, 1.0f, true);
                    state.selectedBodyId = id;
                    state.selectedRopeId = -1;
                    world.updateStaticTelemetry();
                    world.saveSnapshot();
                    break;
                }
                case ToolType::ADD_ROPE: {
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

                            float ropeLen = std::clamp(dist, 0.4f, 7.0f);
                            if (!bodyB.isStatic && bodyB.pos.y - bodyB.height * 0.5f <= world.groundY + 0.1f && dist > state.newRopeDefaultLength) {
                                ropeLen = state.newRopeDefaultLength;
                            }

                            int rId = world.addRope(state.ropeStartBodyId, state.hoveredBodyId,
                                                    state.ropeStartOffset, snapB, ropeLen, 180.0f, false);
                            world.syncRopeLengthToAttachedBody(rId);
                            state.selectedRopeId = rId;
                            state.selectedBodyId = -1;
                            state.ropeStartBodyId = -1;
                            world.saveSnapshot();
                        } else {
                            float dragDist = (state.mouseWorldPos - pA).length();
                            float len = (dragDist > 0.4f) ? std::clamp(dragDist, 0.4f, 7.0f) : state.newRopeDefaultLength;
                            int rId = world.addFreeRope(state.ropeStartBodyId, state.ropeStartOffset, state.mouseWorldPos, len, 180.0f, false);
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

    if (state.draggedBodyId >= 0 && state.draggedBodyId < (int)world.bodies.size()) {
        Body& db = world.bodies[state.draggedBodyId];
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (state.paused || (db.isStatic && !db.supportLocked))) {
            db.pos = state.mouseWorldPos + state.dragOffset;
            db.vel = {0.0f, 0.0f};
            db.angVel = 0.0f;
            world.resolveEditorOverlap(db.id);
            world.tryAttachBodyToFreeRopes(db.id);
        }
    }

    if (state.paused && state.draggedFreeRopeId >= 0 && state.draggedFreeRopeId < (int)world.ropes.size()) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            RopeLink& r = world.ropes[state.draggedFreeRopeId];
            Vec2 pA = world.bodies[r.idA].localToWorld(r.offsetA);
            Vec2 d = state.mouseWorldPos - pA;
            r.restLength = std::clamp(d.length(), 0.3f, 7.0f);
            r.currentLength = r.restLength;
            Vec2 n = (d.length() > 1e-4f) ? d.normalized() : Vec2{0.0f, -1.0f};
            r.freePos = pA + n * r.restLength;
            world.updateStaticTelemetry();
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (state.draggedBodyId != -1 || state.draggedFreeRopeId != -1) {
            if (state.paused) world.saveSnapshot();
        }
        state.draggedBodyId = -1;
        state.draggedFreeRopeId = -1;
    }
}