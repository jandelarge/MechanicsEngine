#include "Renderer.hpp"
#include <vector>
#include <cmath>
#include <cstdio>

static const Color COL_BG_DARK     = {25, 23, 22, 255};    // #191716
static const Color COL_PANEL_BG    = {32, 29, 28, 255};
static const Color COL_WINE        = {119, 51, 68, 255};   // #773344
static const Color COL_WINE_BRIGHT = {148, 66, 86, 255};
static const Color COL_WINE_DIM    = {82, 36, 48, 255};
static const Color COL_GOLD        = {194, 168, 120, 255}; // #C2A878
static const Color COL_GOLD_DIM    = {132, 114, 84, 255};
static const Color COL_TEXT_CREAM  = {236, 228, 216, 255};
static const Color COL_TEXT_MUTED  = {166, 155, 140, 255};

static Font LoadRussianFont(int fontSize) {
    std::vector<int> codepoints;
    for (int i = 32; i <= 126; i++) codepoints.push_back(i);
    for (int i = 0x0400; i <= 0x04FF; i++) codepoints.push_back(i);
    codepoints.push_back(0x0394); // Δ
    codepoints.push_back(0x03BC); // μ
    codepoints.push_back(0x03C9); // ω
    codepoints.push_back(0x00B2); // ²
    codepoints.push_back(0x00B0); // °

    const char* fontPath = "C:\\Windows\\Fonts\\arialbd.ttf";
    if (!FileExists(fontPath)) fontPath = "C:\\Windows\\Fonts\\arial.ttf";

    Font font = LoadFontEx(fontPath, fontSize, codepoints.data(), (int)codepoints.size());
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    return font;
}

void Renderer::init() {
    fontSmall = LoadRussianFont(14);
    fontMain  = LoadRussianFont(15);
    fontBold  = LoadRussianFont(18);
}

void Renderer::unload() {
    UnloadFont(fontSmall);
    UnloadFont(fontMain);
    UnloadFont(fontBold);
}

Vector2 Renderer::worldToScreen(Vec2 wPos, int sw, int sh) const {
    float canvasWidth = (float)(sw - InputHandler::INSPECTOR_WIDTH);
    return {
        canvasWidth * 0.5f + wPos.x * ppm,
        sh * 0.84f - wPos.y * ppm
    };
}

void Renderer::drawButton(Rectangle r, const char* text, bool active, Color accent) const {
    Vector2 m = GetMousePosition();
    bool hovered = (m.x >= r.x && m.x <= r.x + r.width && m.y >= r.y && m.y <= r.y + r.height);

    Color bg = active ? COL_WINE : (hovered ? Color{46, 40, 39, 255} : Color{38, 34, 33, 255});
    Color border = active ? accent : (hovered ? COL_GOLD : Color{68, 60, 56, 255});

    DrawRectangleRounded(r, 0.22f, 6, bg);
    DrawRectangleRoundedLinesEx(r, 0.22f, 6, active ? 1.8f : 1.0f, border);

    Vector2 ts = MeasureTextEx(fontMain, text, 15, 1.0f);
    DrawTextEx(fontMain, text, {r.x + (r.width - ts.x) * 0.5f, r.y + (r.height - ts.y) * 0.5f},
               15, 1.0f, active ? COL_TEXT_CREAM : COL_TEXT_MUTED);
}

void Renderer::drawValueInputRow(float x, float y, const char* label, const char* currentValText,
                                 bool editing, const std::string& inputBuffer) const {
    DrawTextEx(fontMain, label, {x, y}, 15, 1.0f, COL_TEXT_MUTED);

    float by = y + 20.0f;
    float bh = 28.0f;

    Rectangle curRect = {x, by, 118.0f, bh};
    DrawRectangleRounded(curRect, 0.2f, 6, {28, 25, 24, 255});
    DrawRectangleRoundedLinesEx(curRect, 0.2f, 6, 1.0f, {64, 56, 52, 255});
    Vector2 tsCur = MeasureTextEx(fontMain, currentValText, 15, 1.0f);
    DrawTextEx(fontMain, currentValText, {curRect.x + (curRect.width - tsCur.x) * 0.5f, curRect.y + (curRect.height - tsCur.y) * 0.5f},
               15, 1.0f, COL_TEXT_CREAM);

    Rectangle inRect = {x + 126.0f, by, 142.0f, bh};
    Vector2 m = GetMousePosition();
    bool hovered = (m.x >= inRect.x && m.x <= inRect.x + inRect.width && m.y >= inRect.y && m.y <= inRect.y + inRect.height);

    DrawRectangleRounded(inRect, 0.2f, 6, editing ? Color{44, 24, 30, 255} : Color{38, 34, 33, 255});
    DrawRectangleRoundedLinesEx(inRect, 0.2f, 6, editing ? 2.0f : 1.0f,
                                editing ? COL_GOLD : (hovered ? COL_GOLD_DIM : Color{72, 64, 58, 255}));

    const char* boxText = editing ? TextFormat("%s_", inputBuffer.c_str()) : "Ввести...";
    Color txtCol = editing ? COL_GOLD : COL_TEXT_MUTED;
    Vector2 tsIn = MeasureTextEx(fontMain, boxText, 15, 1.0f);
    DrawTextEx(fontMain, boxText, {inRect.x + (inRect.width - tsIn.x) * 0.5f, inRect.y + (inRect.height - tsIn.y) * 0.5f},
               15, 1.0f, txtCol);
}

void Renderer::drawSpringCoil(Vector2 sA, Vector2 sB, Color col, float thickness) const {
    float dx = sB.x - sA.x;
    float dy = sB.y - sA.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 2.0f) return;

    Vector2 u = {dx / len, dy / len};
    Vector2 p = {-u.y, u.x};

    float endLead = std::min(12.0f, len * 0.14f);
    Vector2 cStart = {sA.x + u.x * endLead, sA.y + u.y * endLead};
    Vector2 cEnd   = {sB.x - u.x * endLead, sB.y - u.y * endLead};

    DrawLineEx(sA, cStart, thickness, col);
    DrawLineEx(cEnd, sB, thickness, col);

    const int COILS = 12;
    float coilLen = len - 2.0f * endLead;
    float amp = 6.5f;

    Vector2 prev = cStart;
    for (int i = 1; i <= COILS; ++i) {
        float t = ((float)i - 0.5f) / (float)COILS;
        float side = (i % 2 == 1) ? 1.0f : -1.0f;
        Vector2 pt = {
            cStart.x + u.x * (t * coilLen) + p.x * (amp * side),
            cStart.y + u.y * (t * coilLen) + p.y * (amp * side)
        };
        DrawLineEx(prev, pt, thickness, col);
        prev = pt;
    }
    DrawLineEx(prev, cEnd, thickness, col);

    DrawCircleV(sA, 3.5f, COL_GOLD);
    DrawCircleV(sB, 3.5f, COL_GOLD);
}

void Renderer::drawCreationIcon(int itemIndex, Vector2 center, Color col, bool active) const {
    float cx = center.x;
    float cy = center.y;

    switch (itemIndex) {
        case 0: { // Брусок
            Rectangle box = {cx - 7.5f, cy - 7.5f, 15.0f, 15.0f};
            DrawRectangleRounded(box, 0.2f, 4, Fade(col, active ? 0.35f : 0.15f));
            DrawRectangleRoundedLinesEx(box, 0.2f, 4, 1.8f, col);
            DrawCircleV({cx, cy}, 1.8f, col);
            break;
        }
        case 1: { // Шар
            DrawCircleV({cx, cy}, 8.0f, Fade(col, active ? 0.35f : 0.15f));
            DrawCircleLines((int)cx, (int)cy, 8.0f, col);
            DrawLineEx({cx, cy}, {cx + 5.8f, cy - 5.2f}, 1.8f, col);
            DrawCircleV({cx, cy}, 1.8f, col);
            break;
        }
        case 2: { // Опора
            DrawLineEx({cx - 9.5f, cy + 3.0f}, {cx + 9.5f, cy + 3.0f}, 2.2f, col);
            for (int i = -2; i <= 2; ++i) {
                float sx = cx + i * 4.2f + 1.5f;
                DrawLineEx({sx, cy + 3.0f}, {sx - 3.8f, cy - 3.8f}, 1.5f, col);
            }
            break;
        }
        case 3: { // Нить
            Vector2 a = {cx - 6.0f, cy - 6.0f};
            Vector2 b = {cx + 6.0f, cy + 6.0f};
            DrawLineEx(a, b, 2.0f, col);
            DrawCircleV(a, 3.0f, COL_BG_DARK);
            DrawCircleLines((int)a.x, (int)a.y, 3.0f, col);
            DrawCircleV(b, 3.0f, COL_BG_DARK);
            DrawCircleLines((int)b.x, (int)b.y, 3.0f, col);
            break;
        }
        case 4: { // Пружина (зигзаг)
            Vector2 pts[6] = {
                {cx - 7.0f, cy + 7.0f},
                {cx - 5.5f, cy + 1.5f},
                {cx + 1.5f, cy + 5.5f},
                {cx - 1.5f, cy - 5.5f},
                {cx + 5.5f, cy - 1.5f},
                {cx + 7.0f, cy - 7.0f}
            };
            for (int i = 0; i < 5; ++i) {
                DrawLineEx(pts[i], pts[i + 1], 1.8f, col);
            }
            DrawCircleV(pts[0], 2.2f, col);
            DrawCircleV(pts[5], 2.2f, col);
            break;
        }
    }
}

void Renderer::drawDockIcon(int index, Rectangle r, bool active, bool paused, Color col) const {
    float cx = r.x + r.width * 0.5f;
    float cy = r.y + r.height * 0.5f;

    switch (index) {
        case 0: { // Пуск / Пауза
            if (paused) {
                DrawTriangle({cx - 5.0f, cy - 8.0f}, {cx - 5.0f, cy + 8.0f}, {cx + 8.0f, cy}, col);
            } else {
                DrawRectangleRec({cx - 6.0f, cy - 8.0f, 4.0f, 16.0f}, col);
                DrawRectangleRec({cx + 2.0f, cy - 8.0f, 4.0f, 16.0f}, col);
            }
            break;
        }
        case 1: { // Сброс
            DrawRing({cx, cy}, 6.4f, 8.8f, 42.0f, 312.0f, 28, col);
            Vector2 tip       = {cx + 8.6f, cy - 2.4f};
            Vector2 outerBase = {cx + 7.2f, cy - 9.4f};
            Vector2 innerBase = {cx + 1.8f, cy - 3.2f};
            DrawTriangle(tip, outerBase, innerBase, col);
            break;
        }
        case 2: { // [1] Курсор
            Vector2 p1 = {cx - 6.0f, cy - 10.0f};
            Vector2 p2 = {cx - 6.0f, cy + 7.0f};
            Vector2 p3 = {cx + 6.0f, cy + 1.0f};
            DrawTriangle(p1, p2, p3, col);
            DrawLineEx({cx - 2.0f, cy + 2.0f}, {cx + 3.0f, cy + 9.0f}, 2.5f, col);
            break;
        }
        case 3: { // [2] Плюс [+]
            DrawLineEx({cx - 8.5f, cy}, {cx + 8.5f, cy}, 2.6f, col);
            DrawLineEx({cx, cy - 8.5f}, {cx, cy + 8.5f}, 2.6f, col);
            break;
        }
        case 4: { // [3] Удалить (корзина)
            DrawLineEx({cx - 8.0f, cy - 6.0f}, {cx + 8.0f, cy - 6.0f}, 2.0f, col);
            DrawLineEx({cx - 3.0f, cy - 8.5f}, {cx + 3.0f, cy - 8.5f}, 2.0f, col);
            DrawRectangleLinesEx({cx - 6.5f, cy - 5.0f, 13.0f, 14.0f}, 1.8f, col);
            DrawLineEx({cx - 2.2f, cy - 2.0f}, {cx - 2.2f, cy + 5.5f}, 1.5f, col);
            DrawLineEx({cx + 2.2f, cy - 2.0f}, {cx + 2.2f, cy + 5.5f}, 1.5f, col);
            break;
        }
    }
}

void Renderer::drawLiquidDock(int sh, const AppState& state) const {
    Rectangle dock = InputHandler::getDockRect(sh);

    DrawRectangleRounded({dock.x + 2.0f, dock.y + 5.0f, dock.width, dock.height}, 0.48f, 16, {10, 8, 8, 160});
    DrawRectangleRounded(dock, 0.48f, 16, {34, 29, 28, 238});
    DrawRectangleRoundedLinesEx(dock, 0.48f, 16, 1.5f, Fade(COL_GOLD, 0.45f));

    Rectangle b1 = InputHandler::getDockButtonRect(1, sh);
    float divX = b1.x + b1.width + 9.0f;
    DrawLineEx({divX, dock.y + 12.0f}, {divX, dock.y + dock.height - 12.0f}, 1.5f, Fade(COL_GOLD, 0.25f));

    bool isCreationTool = (state.activeTool == ToolType::ADD_BLOCK ||
                           state.activeTool == ToolType::ADD_CIRCLE ||
                           state.activeTool == ToolType::ADD_SUPPORT ||
                           state.activeTool == ToolType::ADD_ROPE ||
                           state.activeTool == ToolType::ADD_SPRING);

    const char* activeCreateName = "Создать объект [2]";
    if (state.activeTool == ToolType::ADD_BLOCK)   activeCreateName = "Создать: Брусок [2]";
    if (state.activeTool == ToolType::ADD_CIRCLE)  activeCreateName = "Создать: Шар [2]";
    if (state.activeTool == ToolType::ADD_SUPPORT) activeCreateName = "Создать: Опора [2]";
    if (state.activeTool == ToolType::ADD_ROPE)    activeCreateName = "Создать: Нить [2]";
    if (state.activeTool == ToolType::ADD_SPRING)  activeCreateName = "Создать: Пружина [2]";

    const char* tooltips[5] = {
        state.paused ? "Запуск [Пробел]" : "Пауза [Пробел]",
        "Сброс [R]",
        "Курсор [1]",
        activeCreateName,
        "Удалить [3]"
    };

    for (int i = 0; i < 5; ++i) {
        Rectangle btn = InputHandler::getDockButtonRect(i, sh);
        bool hovered = (state.hoveredDockBtn == i);
        bool active = false;

        if (i == 0) active = !state.paused;
        else if (i == 2) active = (state.activeTool == ToolType::SELECT_MOVE && !state.addMenuOpen);
        else if (i == 3) active = state.addMenuOpen || isCreationTool;
        else if (i == 4) active = (state.activeTool == ToolType::DELETE_OBJ && !state.addMenuOpen);

        if (active) {
            DrawRectangleRounded(btn, 0.36f, 10, COL_WINE);
            DrawRectangleRoundedLinesEx(btn, 0.36f, 10, 1.8f, COL_GOLD);
        } else if (hovered) {
            DrawRectangleRounded(btn, 0.36f, 10, Fade(COL_WINE, 0.45f));
            DrawRectangleRoundedLinesEx(btn, 0.36f, 10, 1.2f, Fade(COL_GOLD, 0.65f));
        }

        Color iconCol = active ? COL_GOLD : (hovered ? COL_TEXT_CREAM : COL_TEXT_MUTED);
        drawDockIcon(i, btn, active, state.paused, iconCol);

        if (i >= 2) {
            const char* keyTxt = TextFormat("%d", i - 1);
            DrawTextEx(fontSmall, keyTxt, {btn.x + btn.width - 11.0f, btn.y + btn.height - 14.0f},
                       14, 1.0f, active ? COL_TEXT_CREAM : COL_GOLD_DIM);
        }

        if (hovered && !state.addMenuOpen) {
            Vector2 ts = MeasureTextEx(fontMain, tooltips[i], 14, 1.0f);
            float pillW = ts.x + 20.0f;
            float pillH = 26.0f;
            float pillX = btn.x + btn.width * 0.5f - pillW * 0.5f;
            if (pillX < 12.0f) pillX = 12.0f;
            float pillY = dock.y - 34.0f;

            DrawRectangleRounded({pillX, pillY, pillW, pillH}, 0.5f, 8, {34, 28, 27, 240});
            DrawRectangleRoundedLinesEx({pillX, pillY, pillW, pillH}, 0.5f, 8, 1.2f, COL_GOLD);
            DrawTextEx(fontMain, tooltips[i], {pillX + 10.0f, pillY + (pillH - ts.y) * 0.5f}, 14, 1.0f, COL_TEXT_CREAM);
        }
    }

    // Всплывающее меню [+] с 5 объектами [1..5]
    if (state.addMenuOpen) {
        Rectangle menu = InputHandler::getAddMenuRect(sh);

        DrawRectangleRounded({menu.x + 2.0f, menu.y + 4.0f, menu.width, menu.height}, 0.35f, 12, {10, 8, 8, 170});
        DrawRectangleRounded(menu, 0.35f, 12, {36, 30, 29, 242});
        DrawRectangleRoundedLinesEx(menu, 0.35f, 12, 1.6f, COL_GOLD);

        const char* names[5] = {"Брусок", "Шар", "Опора", "Нить", "Пружина"};
        ToolType types[5] = {
            ToolType::ADD_BLOCK,
            ToolType::ADD_CIRCLE,
            ToolType::ADD_SUPPORT,
            ToolType::ADD_ROPE,
            ToolType::ADD_SPRING
        };

        for (int i = 0; i < 5; ++i) {
            Rectangle item = InputHandler::getAddMenuItemRect(i, sh);
            bool hovered = (state.hoveredAddMenuBtn == i);
            bool active = (state.activeTool == types[i]);

            Color bg = active ? COL_WINE : (hovered ? Fade(COL_WINE, 0.55f) : Color{29, 25, 24, 230});
            Color border = active ? COL_GOLD : (hovered ? COL_GOLD : Color{68, 58, 54, 255});

            DrawRectangleRounded(item, 0.28f, 8, bg);
            DrawRectangleRoundedLinesEx(item, 0.28f, 8, active ? 1.8f : 1.0f, border);

            Color col = active ? COL_GOLD : (hovered ? COL_TEXT_CREAM : COL_TEXT_MUTED);
            drawCreationIcon(i, {item.x + 17.0f, item.y + item.height * 0.5f}, col, active);

            DrawTextEx(fontSmall, names[i], {item.x + 29.0f, item.y + 18.0f}, 14, 1.0f,
                       active ? COL_TEXT_CREAM : (hovered ? COL_TEXT_CREAM : COL_TEXT_MUTED));

            const char* keyStr = TextFormat("[%d]", i + 1);
            DrawTextEx(fontSmall, keyStr, {item.x + item.width - 21.0f, item.y + 3.0f}, 14, 1.0f, COL_GOLD);
        }
    }
}

void Renderer::drawInspector(const PhysicsWorld& world, const AppState& state, int sw, int sh) const {
    float panelX = (float)(sw - InputHandler::INSPECTOR_WIDTH);
    float topY = 12.0f;

    DrawRectangle((int)panelX, 0, InputHandler::INSPECTOR_WIDTH, sh, COL_PANEL_BG);
    DrawLine((int)panelX, 0, (int)panelX, sh, {68, 56, 52, 255});

    float x = panelX + 16.0f;
    float w = InputHandler::INSPECTOR_WIDTH - 32.0f;

    if (state.selectedBodyId >= 0 && state.selectedBodyId < (int)world.bodies.size()) {
        const Body& b = world.bodies[state.selectedBodyId];

        if (b.isStatic) {
            DrawTextEx(fontBold, TextFormat("Свойства опоры #%d", b.id + 1), {x, topY + 12}, 18, 1.0f, COL_GOLD);
            drawButton({x, topY + 44, w, 30},
                       b.supportLocked ? "Режим: Зафиксирована" : "Режим: Перемещаемая",
                       !b.supportLocked, COL_GOLD);

            drawValueInputRow(x, topY + 84,  "Позиция X", TextFormat("%.2f м", b.pos.x),
                              state.editingField == EditField::SUPPORT_X, state.inputBuffer);
            drawValueInputRow(x, topY + 138, "Позиция Y", TextFormat("%.2f м", b.pos.y),
                              state.editingField == EditField::SUPPORT_Y, state.inputBuffer);
            drawValueInputRow(x, topY + 192, "Ширина опоры W", TextFormat("%.2f м", b.width),
                              state.editingField == EditField::SUPPORT_WIDTH, state.inputBuffer);
            drawValueInputRow(x, topY + 246, "Угол наклона (горка)", TextFormat("%.1f\xC2\xB0", b.angle * 57.2958f),
                              state.editingField == EditField::SUPPORT_ANGLE, state.inputBuffer);

            drawButton({x, topY + 302, w, 26}, "Выровнять горизонтально (0\xC2\xB0)", false, COL_GOLD);
            DrawTextEx(fontSmall, "* Колёсико мыши — быстрый наклон", {x, topY + 336}, 14, 1.0f, COL_TEXT_MUTED);

            drawButton({x, (float)sh - 48.0f, w, 32}, "Удалить опору", false, COL_WINE_BRIGHT);
            return;
        } else if (b.shape == ShapeType::CIRCLE) {
            DrawTextEx(fontBold, TextFormat("Свойства шара #%d", b.id + 1), {x, topY + 12}, 18, 1.0f, COL_GOLD);

            drawValueInputRow(x, topY + 44,  "Масса m", TextFormat("%.1f кг", b.mass),
                              state.editingField == EditField::BODY_MASS, state.inputBuffer);
            drawValueInputRow(x, topY + 98,  "Коэффициент трения \xCE\xBC", TextFormat("%.2f", b.friction),
                              state.editingField == EditField::BODY_FRICTION, state.inputBuffer);
            drawValueInputRow(x, topY + 152, "Радиус шара R", TextFormat("%.2f м", b.radius),
                              state.editingField == EditField::BODY_RADIUS, state.inputBuffer);
            drawValueInputRow(x, topY + 206, "Упругость отскока e", TextFormat("%.2f", b.restitution),
                              state.editingField == EditField::BODY_RESTITUTION, state.inputBuffer);
            drawValueInputRow(x, topY + 260, "Внешняя сила тяги F", TextFormat("%.1f Н", b.appliedForceX),
                              state.editingField == EditField::BODY_FORCE_X, state.inputBuffer);

            drawButton({x, topY + 316, w, 26},
                       b.canRotate ? "Вращение шара: Вкл" : "Вращение шара: Выкл",
                       b.canRotate, COL_GOLD);
            drawButton({x, topY + 348, w, 26}, "Сбросить угол (0\xC2\xB0)", false, COL_GOLD);

            float ty = topY + 382;
            DrawLine((int)x, (int)ty, (int)(x + w), (int)ty, {64, 54, 50, 255});
            DrawTextEx(fontBold, "Телеметрия", {x, ty + 8}, 17, 1.0f, COL_GOLD);
            DrawTextEx(fontMain, TextFormat("Момент инерции I: %.2f кг*м\xC2\xB2", b.inertia), {x, ty + 30}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Угол / \xCF\x89: %.1f\xC2\xB0 (%.2f рад/с)", b.angle * 57.2958f, b.angVel), {x, ty + 49}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Скорость |v|: %.2f м/с", b.vel.length()), {x, ty + 68}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Сила тяжести mg: %.1f Н", b.mass * world.G), {x, ty + 87}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Реакция опоры N: %.1f Н", b.normalForce), {x, ty + 106}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Давление сверху P: %.1f Н", b.topLoadForce), {x, ty + 125}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Сила трения Fтр: %.1f Н", b.frictionForce), {x, ty + 144}, 15, 1.0f, COL_TEXT_MUTED);

            drawButton({x, (float)sh - 48.0f, w, 32}, "Удалить шар", false, COL_WINE_BRIGHT);
            return;
        } else {
            DrawTextEx(fontBold, TextFormat("Свойства бруска #%d", b.id + 1), {x, topY + 12}, 18, 1.0f, COL_GOLD);

            drawValueInputRow(x, topY + 44,  "Масса m", TextFormat("%.1f кг", b.mass),
                              state.editingField == EditField::BODY_MASS, state.inputBuffer);
            drawValueInputRow(x, topY + 98,  "Коэффициент трения \xCE\xBC", TextFormat("%.2f", b.friction),
                              state.editingField == EditField::BODY_FRICTION, state.inputBuffer);
            drawValueInputRow(x, topY + 152, "Ширина W", TextFormat("%.2f м", b.width),
                              state.editingField == EditField::BODY_WIDTH, state.inputBuffer);
            drawValueInputRow(x, topY + 206, "Высота H", TextFormat("%.2f м", b.height),
                              state.editingField == EditField::BODY_HEIGHT, state.inputBuffer);
            drawValueInputRow(x, topY + 260, "Внешняя сила тяги F", TextFormat("%.1f Н", b.appliedForceX),
                              state.editingField == EditField::BODY_FORCE_X, state.inputBuffer);

            drawButton({x, topY + 316, w, 26},
                       b.canRotate ? "Инерция вращения: Вкл" : "Инерция вращения: Выкл",
                       b.canRotate, COL_GOLD);
            drawButton({x, topY + 348, w, 26}, "Выровнять угол (0\xC2\xB0)", false, COL_GOLD);

            float ty = topY + 382;
            DrawLine((int)x, (int)ty, (int)(x + w), (int)ty, {64, 54, 50, 255});
            DrawTextEx(fontBold, "Телеметрия", {x, ty + 8}, 17, 1.0f, COL_GOLD);
            DrawTextEx(fontMain, TextFormat("Момент инерции I: %.2f кг*м\xC2\xB2", b.inertia), {x, ty + 30}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Угол / \xCF\x89: %.1f\xC2\xB0 (%.2f рад/с)", b.angle * 57.2958f, b.angVel), {x, ty + 49}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Скорость |v|: %.2f м/с", b.vel.length()), {x, ty + 68}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Сила тяжести mg: %.1f Н", b.mass * world.G), {x, ty + 87}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Реакция опоры N: %.1f Н", b.normalForce), {x, ty + 106}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Давление сверху P: %.1f Н", b.topLoadForce), {x, ty + 125}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Сила трения Fтр: %.1f Н", b.frictionForce), {x, ty + 144}, 15, 1.0f, COL_TEXT_MUTED);

            drawButton({x, (float)sh - 48.0f, w, 32}, "Удалить брусок", false, COL_WINE_BRIGHT);
            return;
        }
    }

    if (state.selectedRopeId >= 0 && state.selectedRopeId < (int)world.ropes.size()) {
        const RopeLink& r = world.ropes[state.selectedRopeId];

        DrawTextEx(fontBold, TextFormat(r.isSpring ? "Свойства пружины #%d" : "Свойства нити #%d", r.id + 1),
                   {x, topY + 12}, 18, 1.0f, COL_GOLD);
        drawButton({x, topY + 44, w, 30},
                   r.isRigid ? (r.isSpring ? "Режим: Жесткий стержень" : "Режим: Нерастяжимая")
                             : (r.isSpring ? "Режим: Пружина Гука (2-стор.)" : "Режим: Упругая (Гук)"),
                   true, COL_GOLD);

        drawValueInputRow(x, topY + 84, "Свободная длина L0", TextFormat("%.2f м", r.restLength),
                          state.editingField == EditField::ROPE_LENGTH, state.inputBuffer);

        drawButton({x, topY + 140, w, 28}, "Выровнять груз к длине L0", false, COL_GOLD);

        if (!r.isRigid) {
            drawValueInputRow(x, topY + 178, "Жесткость k", TextFormat("%.0f Н/м", r.stiffness),
                              state.editingField == EditField::ROPE_STIFFNESS, state.inputBuffer);
        }

        if (r.idB != -1) {
            drawButton({x, topY + 236, w, 28}, "Отцепить груз", false, COL_GOLD);
        }

        float ty = topY + 276;
        DrawLine((int)x, (int)ty, (int)(x + w), (int)ty, {64, 54, 50, 255});
        DrawTextEx(fontBold, r.isSpring ? "Параметры пружины" : "Параметры троса", {x, ty + 10}, 17, 1.0f, COL_GOLD);
        DrawTextEx(fontMain, TextFormat(r.isSpring ? "Сила упругости Fупр: %.1f Н" : "Натяжение T: %.1f Н", r.tension),
                   {x, ty + 36}, 15, 1.0f, COL_TEXT_MUTED);
        DrawTextEx(fontMain, TextFormat("Деформация \xCE\x94L: %+.1f см", r.signedExtension() * 100.0f),
                   {x, ty + 58}, 15, 1.0f, COL_TEXT_MUTED);
        DrawTextEx(fontMain, TextFormat("Текущая длина L: %.2f м", r.currentLength), {x, ty + 80}, 15, 1.0f, COL_TEXT_MUTED);
        DrawTextEx(fontMain, TextFormat("Энергия Ep: %.2f Дж", r.potentialEnergy()), {x, ty + 102}, 15, 1.0f, COL_TEXT_MUTED);
        DrawTextEx(fontMain, (r.idB == -1) ? "Статус: Свободный конец" : "Статус: Груз закреплен", {x, ty + 124}, 15, 1.0f,
                   (r.idB == -1) ? COL_GOLD : COL_TEXT_CREAM);

        drawButton({x, (float)sh - 48.0f, w, 32}, r.isSpring ? "Удалить пружину" : "Удалить нить", false, COL_WINE_BRIGHT);
        return;
    }

    DrawTextEx(fontBold, "Параметры среды", {x, topY + 12}, 18, 1.0f, COL_GOLD);

    drawValueInputRow(x, topY + 44, "Ускорение g", TextFormat("%.2f м/с\xC2\xB2", world.G),
                      state.editingField == EditField::WORLD_GRAVITY, state.inputBuffer);

    drawButton({x, topY + 98, 128, 26}, "g = 9.81", std::abs(world.G - 9.81f) < 0.02f, COL_GOLD);
    drawButton({x + 140, topY + 98, 128, 26}, "g = 10.0", std::abs(world.G - 10.0f) < 0.02f, COL_GOLD);

    drawValueInputRow(x, topY + 134, "Точность расчета", TextFormat("%d шаг.", world.substeps),
                      state.editingField == EditField::WORLD_SUBSTEPS, state.inputBuffer);

    drawValueInputRow(x, topY + 188, "Длина новой связи L0", TextFormat("%.2f м", state.newRopeDefaultLength),
                      state.editingField == EditField::NEW_ROPE_LENGTH, state.inputBuffer);

    drawButton({x, topY + 250, w, 30}, "Сбросить к демо-сцене", false, COL_GOLD);
    drawButton({x, topY + 288, w, 30}, "Очистить всю сцену", false, COL_GOLD);
}

void Renderer::drawHatchedSupport(const Body& b, int sw, int sh, bool highlighted, bool unlocked) const {
    Vec2 leftW = b.localToWorld({-b.width * 0.5f, 0.0f});
    Vec2 rightW = b.localToWorld({b.width * 0.5f, 0.0f});
    Vector2 p1 = worldToScreen(leftW, sw, sh);
    Vector2 p2 = worldToScreen(rightW, sw, sh);

    Color lineCol = highlighted ? COL_GOLD : (unlocked ? COL_WINE_BRIGHT : COL_TEXT_CREAM);
    DrawLineEx(p1, p2, highlighted ? 3.5f : 3.0f, lineCol);

    int steps = std::max(4, (int)(b.width * 5.0f));
    for (int i = 0; i <= steps; ++i) {
        float t = (float)i / (float)steps;
        Vec2 baseW = b.localToWorld({-b.width * 0.5f + t * b.width, 0.0f});
        Vec2 tipW  = b.localToWorld({-b.width * 0.5f + t * b.width - 0.14f, 0.14f});
        DrawLineEx(worldToScreen(baseW, sw, sh), worldToScreen(tipW, sw, sh),
                   1.5f, highlighted ? COL_GOLD : COL_GOLD_DIM);
    }
}

void Renderer::drawRotatedBlock(const Body& b, Vector2 scr, float bw, float bh, bool isSelected, bool isActive) const {
    Color bodyCol = isActive ? COL_WINE_BRIGHT
                  : (isSelected ? Color{134, 58, 77, 255} : COL_WINE);
    Color outlineCol = isSelected ? COL_GOLD : COL_TEXT_CREAM;

    float deg = -b.angle * 57.29578f;
    DrawRectanglePro({scr.x, scr.y, bw, bh}, {bw * 0.5f, bh * 0.5f}, deg, bodyCol);

    auto worldVerts = b.getVertices();
    Vector2 scrCorners[4];
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    for (int i = 0; i < 4; ++i) {
        scrCorners[i] = worldToScreen(worldVerts[i], sw, sh);
    }
    for (int i = 0; i < 4; ++i) {
        DrawLineEx(scrCorners[i], scrCorners[(i + 1) % 4], isSelected ? 2.5f : 1.6f, outlineCol);
    }

    const char* massTxt = TextFormat("%.1f кг", b.mass);
    Vector2 txtSize = MeasureTextEx(fontBold, massTxt, 18, 1.0f);
    DrawTextEx(fontBold, massTxt, {scr.x - txtSize.x * 0.5f, scr.y - txtSize.y * 0.5f}, 18, 1.0f, COL_TEXT_CREAM);
}

void Renderer::drawVectorArrow(Vector2 start, Vector2 end, Color color, const char* label, Vector2 labelOffset) const {
    DrawLineEx(start, end, 2.5f, color);

    Vector2 dir = {end.x - start.x, end.y - start.y};
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 4.0f) {
        Vector2 n = {dir.x / len, dir.y / len};
        Vector2 p = {-n.y, n.x};
        Vector2 a1 = {end.x - n.x * 9.0f + p.x * 4.5f, end.y - n.y * 9.0f + p.y * 4.5f};
        Vector2 a2 = {end.x - n.x * 9.0f - p.x * 4.5f, end.y - n.y * 9.0f - p.y * 4.5f};
        DrawTriangle(end, a2, a1, color);
    }

    DrawCircleV(start, 3.5f, color);
    DrawTextEx(fontMain, label, {end.x + labelOffset.x, end.y + labelOffset.y}, 15, 1.0f, color);
}

void Renderer::drawRopeTooltip(const RopeLink& r, Vector2 midScreen) const {
    float boxX = midScreen.x + 16.0f;
    float boxY = midScreen.y - 55.0f;
    float boxW = 256.0f;
    float boxH = 132.0f;

    DrawRectangleRounded({boxX, boxY, boxW, boxH}, 0.16f, 8, {32, 28, 27, 240});
    DrawRectangleRoundedLinesEx({boxX, boxY, boxW, boxH}, 0.16f, 8, 1.5f, COL_GOLD);

    DrawTextEx(fontBold, r.isSpring ? "Параметры пружины" : "Параметры троса", {boxX + 12, boxY + 8}, 17, 1.0f, COL_GOLD);
    DrawTextEx(fontMain, TextFormat(r.isSpring ? "Сила Fупр: %.1f Н" : "Натяжение T: %.1f Н", r.tension), {boxX + 12, boxY + 32}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, TextFormat("Деформация \xCE\x94L: %+.1f см", r.signedExtension() * 100.0f), {boxX + 12, boxY + 52}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, TextFormat("Длина L: %.2f м (L0: %.2f м)", r.currentLength, r.restLength), {boxX + 12, boxY + 72}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, TextFormat("Энергия Ep: %.2f Дж", r.potentialEnergy()), {boxX + 12, boxY + 92}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, r.isRigid ? (r.isSpring ? "Тип: Жесткий стержень" : "Тип: Нерастяжимая нить")
                                   : TextFormat("Жесткость k: %.0f Н/м", r.stiffness),
               {boxX + 12, boxY + 110}, 15, 1.0f, COL_TEXT_MUTED);
}

void Renderer::draw(const PhysicsWorld& world, const AppState& state) {
    BeginDrawing();
    ClearBackground(COL_BG_DARK);

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int canvasW = sw - InputHandler::INSPECTOR_WIDTH;

    // Пол
    float groundScrY = sh * 0.84f - world.groundY * ppm;
    DrawLine(0, (int)groundScrY, canvasW, (int)groundScrY, COL_GOLD_DIM);

    // 1. Отрисовка тросов и витых пружин
    for (const auto& r : world.ropes) {
        const Body& a = world.bodies[r.idA];
        Vector2 sA = worldToScreen(a.localToWorld(r.offsetA), sw, sh);
        Vector2 sB = worldToScreen(world.getRopeEndWorldPos(r), sw, sh);

        bool isHighlighted = (r.id == state.hoveredRopeId) || (r.id == state.selectedRopeId);
        Color ropeCol = isHighlighted ? COL_TEXT_CREAM : ((r.tension > 0.5f) ? COL_GOLD : COL_GOLD_DIM);

        if (r.isSpring && !r.isRigid) {
            if (!isHighlighted && r.signedExtension() < -0.02f) {
                ropeCol = {222, 146, 96, 255}; // Теплый оттенок при сжатии пружины
            }
            drawSpringCoil(sA, sB, ropeCol, isHighlighted ? 3.0f : 2.2f);
        } else {
            DrawLineEx(sA, sB, isHighlighted ? 4.5f : 3.0f, ropeCol);
        }

        if (r.idB == -1) {
            DrawCircleV(sB, 6.0f, COL_BG_DARK);
            DrawCircleLines((int)sB.x, (int)sB.y, 6.0f, isHighlighted ? COL_TEXT_CREAM : COL_GOLD);
        }
    }

    if ((state.activeTool == ToolType::ADD_ROPE || state.activeTool == ToolType::ADD_SPRING) &&
        state.ropeStartBodyId >= 0 && state.ropeStartBodyId < (int)world.bodies.size()) {
        Vector2 sA = worldToScreen(world.bodies[state.ropeStartBodyId].localToWorld(state.ropeStartOffset), sw, sh);
        Vector2 sB = GetMousePosition();
        if (state.activeTool == ToolType::ADD_SPRING) {
            drawSpringCoil(sA, sB, COL_GOLD, 2.2f);
        } else {
            DrawLineEx(sA, sB, 2.5f, COL_GOLD);
            DrawCircleV(sA, 5.0f, COL_GOLD);
        }

        float dragLen = (state.mouseWorldPos - world.bodies[state.ropeStartBodyId].localToWorld(state.ropeStartOffset)).length();
        DrawTextEx(fontMain, TextFormat("L0 = %.2f м", dragLen), {sB.x + 12, sB.y - 10}, 15, 1.0f, COL_GOLD);
    }

    // 2. Отрисовка тел
    for (const auto& b : world.bodies) {
        Vector2 scr = worldToScreen(b.pos, sw, sh);
        float bw = b.width * ppm;
        float bh = b.height * ppm;
        bool isSelected = (b.id == state.hoveredBodyId) || (b.id == state.draggedBodyId) || (b.id == state.selectedBodyId);

        if (b.isStatic && b.shape == ShapeType::BOX) {
            drawHatchedSupport(b, sw, sh, isSelected, !b.supportLocked);
        } else if (b.shape == ShapeType::CIRCLE) {
            Color bodyCol = (b.id == state.selectedBodyId) ? COL_WINE_BRIGHT : (isSelected ? Color{134, 58, 77, 255} : COL_WINE);
            Color outlineCol = isSelected ? COL_GOLD : COL_TEXT_CREAM;

            float rScreen = b.radius * ppm;
            DrawCircleV(scr, rScreen, bodyCol);

            Vec2 dir1 = {std::cos(b.angle), std::sin(b.angle)};
            Vec2 dir2 = {-dir1.y, dir1.x};
            DrawLineEx(worldToScreen(b.pos - dir1 * b.radius, sw, sh),
                       worldToScreen(b.pos + dir1 * b.radius, sw, sh), 1.5f, Fade(COL_GOLD, 0.35f));
            DrawLineEx(worldToScreen(b.pos - dir2 * b.radius, sw, sh),
                       worldToScreen(b.pos + dir2 * b.radius, sw, sh), 1.5f, Fade(COL_GOLD, 0.35f));

            Vector2 edgeScr = worldToScreen(b.pos + dir1 * b.radius, sw, sh);
            DrawLineEx(scr, edgeScr, 2.4f, outlineCol);
            DrawCircleV(edgeScr, 3.5f, COL_GOLD);

            DrawRing(scr, rScreen - (isSelected ? 2.5f : 1.6f), rScreen, 0.0f, 360.0f, 48, outlineCol);

            const char* massTxt = TextFormat("%.1f кг", b.mass);
            Vector2 txtSize = MeasureTextEx(fontBold, massTxt, 18, 1.0f);
            DrawRectangleRounded({scr.x - txtSize.x * 0.5f - 4.0f, scr.y - txtSize.y * 0.5f - 1.0f, txtSize.x + 8.0f, txtSize.y + 2.0f},
                                 0.4f, 4, Fade(COL_BG_DARK, 0.65f));
            DrawTextEx(fontBold, massTxt, {scr.x - txtSize.x * 0.5f, scr.y - txtSize.y * 0.5f}, 18, 1.0f, COL_TEXT_CREAM);
        } else {
            drawRotatedBlock(b, scr, bw, bh, isSelected, b.id == state.selectedBodyId);
        }

        if ((state.activeTool == ToolType::ADD_ROPE || state.activeTool == ToolType::ADD_SPRING) && b.id == state.hoveredBodyId) {
            Vec2 snap = world.getSnapOffset(b.id, state.mouseWorldPos);
            Vector2 snapScr = worldToScreen(b.localToWorld(snap), sw, sh);
            DrawCircleV(snapScr, 5.5f, COL_GOLD);
        }
    }

    // 2.5. Фантом-превью и индикатор рядом с курсором при спавне
    Vector2 m = GetMousePosition();
    Rectangle dockRect = InputHandler::getDockRect(sh);
    Rectangle menuRect = InputHandler::getAddMenuRect(sh);
    bool overUI = (m.x >= canvasW) ||
                  (m.x >= dockRect.x && m.x <= dockRect.x + dockRect.width && m.y >= dockRect.y && m.y <= dockRect.y + dockRect.height) ||
                  (state.addMenuOpen && m.x >= menuRect.x && m.x <= menuRect.x + menuRect.width && m.y >= menuRect.y && m.y <= menuRect.y + menuRect.height);

    bool isCreationTool = (state.activeTool == ToolType::ADD_BLOCK ||
                           state.activeTool == ToolType::ADD_CIRCLE ||
                           state.activeTool == ToolType::ADD_SUPPORT ||
                           state.activeTool == ToolType::ADD_ROPE ||
                           state.activeTool == ToolType::ADD_SPRING);

    if (state.creatingSupportBodyId >= 0 && state.creatingSupportBodyId < (int)world.bodies.size()) {
        // Живая индикация длины и наклона при растягивании опоры
        const Body& sb = world.bodies[state.creatingSupportBodyId];
        Vector2 p1 = worldToScreen(sb.localToWorld({-sb.width * 0.5f, 0.0f}), sw, sh);
        Vector2 p2 = worldToScreen(sb.localToWorld({ sb.width * 0.5f, 0.0f}), sw, sh);
        DrawCircleV(p1, 5.0f, COL_GOLD);
        DrawCircleV(p2, 5.0f, COL_GOLD);

        const char* info = TextFormat("W = %.2f м | %.1f\xC2\xB0", sb.width, sb.angle * 57.2958f);
        Vector2 ts = MeasureTextEx(fontMain, info, 15, 1.0f);
        Rectangle pill = {m.x + 14.0f, m.y - 32.0f, ts.x + 20.0f, 26.0f};
        DrawRectangleRounded(pill, 0.5f, 6, {28, 24, 23, 235});
        DrawRectangleRoundedLinesEx(pill, 0.5f, 6, 1.2f, COL_GOLD);
        DrawTextEx(fontMain, info, {pill.x + 10.0f, pill.y + (pill.height - ts.y) * 0.5f}, 15, 1.0f, COL_TEXT_CREAM);
    } else if (!overUI && isCreationTool) {
        if (state.activeTool == ToolType::ADD_BLOCK) {
            float s = 1.0f * ppm;
            Rectangle r = {m.x - s * 0.5f, m.y - s * 0.5f, s, s};
            DrawRectangleRec(r, Fade(COL_WINE, 0.28f));
            DrawRectangleLinesEx(r, 1.5f, Fade(COL_GOLD, 0.7f));
        } else if (state.activeTool == ToolType::ADD_CIRCLE) {
            float rad = 0.5f * ppm;
            DrawCircleV(m, rad, Fade(COL_WINE, 0.28f));
            DrawRing(m, rad - 1.5f, rad, 0.0f, 360.0f, 36, Fade(COL_GOLD, 0.7f));
        } else if (state.activeTool == ToolType::ADD_SUPPORT) {
            float w = 3.0f * ppm;
            DrawLineEx({m.x - w * 0.5f, m.y}, {m.x + w * 0.5f, m.y}, 2.5f, Fade(COL_GOLD, 0.75f));
        }

        int iconIdx = 0;
        const char* label = "+ Брусок";
        if (state.activeTool == ToolType::ADD_CIRCLE)  { iconIdx = 1; label = "+ Шар"; }
        if (state.activeTool == ToolType::ADD_SUPPORT) { iconIdx = 2; label = "+ Опора (тяни ЛКМ)"; }
        if (state.activeTool == ToolType::ADD_ROPE)    { iconIdx = 3; label = "+ Нить"; }
        if (state.activeTool == ToolType::ADD_SPRING)  { iconIdx = 4; label = "+ Пружина"; }

        Vector2 ts = MeasureTextEx(fontSmall, label, 14, 1.0f);
        Rectangle tag = {m.x + 14.0f, m.y + 14.0f, ts.x + 34.0f, 24.0f};
        DrawRectangleRounded(tag, 0.5f, 6, {28, 24, 23, 225});
        DrawRectangleRoundedLinesEx(tag, 0.5f, 6, 1.0f, Fade(COL_GOLD, 0.75f));
        drawCreationIcon(iconIdx, {tag.x + 14.0f, tag.y + 12.0f}, COL_GOLD, true);
        DrawTextEx(fontSmall, label, {tag.x + 26.0f, tag.y + (tag.height - ts.y) * 0.5f}, 14, 1.0f, COL_TEXT_CREAM);
    }

    // 3. Векторы сил (mg, N, P, внешняя тяга F и трение Fтр)
    const float FORCE_SCALE = 0.9f;
    if (state.forceViewMode != 2) {
        for (const auto& b : world.bodies) {
            if (b.isStatic) continue;
            bool isHighlighted = (b.id == state.hoveredBodyId) || (b.id == state.draggedBodyId);
            if (state.forceViewMode == 0 && !isHighlighted) continue;

            Vector2 scr = worldToScreen(b.pos, sw, sh);
            float bw = (b.shape == ShapeType::CIRCLE ? (b.radius * 2.0f) : b.width) * ppm;
            float bh = (b.shape == ShapeType::CIRCLE ? (b.radius * 2.0f) : b.height) * ppm;

            float fg = b.mass * world.G;
            drawVectorArrow(scr, {scr.x, scr.y + fg * FORCE_SCALE}, {214, 92, 92, 255},
                            TextFormat("mg = %.1f Н", fg), {10.0f, -8.0f});

            if (b.normalForce > 0.2f) {
                Vector2 bottomPt = {scr.x - 6.0f, scr.y + bh * 0.5f};
                drawVectorArrow(bottomPt, {bottomPt.x, bottomPt.y - b.normalForce * FORCE_SCALE},
                                {126, 186, 132, 255}, TextFormat("N = %.1f Н", b.normalForce), {-95.0f, -8.0f});
            }

            if (b.topLoadForce > 0.2f) {
                Vector2 topPt = {scr.x + 6.0f, scr.y - bh * 0.5f};
                drawVectorArrow(topPt, {topPt.x, topPt.y + b.topLoadForce * FORCE_SCALE},
                                {218, 144, 88, 255}, TextFormat("P = %.1f Н", b.topLoadForce), {10.0f, -8.0f});
            }

            // Вектор внешней силы тяги F
            if (std::abs(b.appliedForceX) > 0.1f) {
                float sign = (b.appliedForceX > 0.0f) ? 1.0f : -1.0f;
                Vector2 sidePt = {scr.x + sign * bw * 0.5f, scr.y};
                Vector2 endPt  = {sidePt.x + b.appliedForceX * FORCE_SCALE, sidePt.y};
                drawVectorArrow(sidePt, endPt, {96, 178, 224, 255},
                                TextFormat("F = %.1f Н", std::abs(b.appliedForceX)),
                                {sign > 0 ? 8.0f : -80.0f, -18.0f});
            }

            // Вектор силы трения Fтр вдоль нижней грани
            if (b.frictionForce > 0.2f) {
                float dirX = (std::abs(b.vel.x) > 0.05f) ? ((b.vel.x > 0.0f) ? -1.0f : 1.0f)
                           : ((std::abs(b.appliedForceX) > 0.1f) ? ((b.appliedForceX > 0.0f) ? -1.0f : 1.0f) : -1.0f);
                Vector2 frStart = {scr.x, scr.y + bh * 0.5f - 2.0f};
                Vector2 frEnd   = {frStart.x + dirX * b.frictionForce * FORCE_SCALE, frStart.y};
                drawVectorArrow(frStart, frEnd, {188, 122, 214, 255},
                                TextFormat("Fтр = %.1f Н", b.frictionForce),
                                {dirX > 0 ? 8.0f : -90.0f, 6.0f});
            }
        }
    }

    // 4. Векторы натяжения/упругости и карточка связи
    for (const auto& r : world.ropes) {
        const Body& a = world.bodies[r.idA];
        bool bodyHighlighted = (r.idB != -1) && ((r.idB == state.hoveredBodyId) || (r.idB == state.draggedBodyId));
        bool ropeHighlighted = (r.id == state.hoveredRopeId);

        if (r.idB != -1 && state.forceViewMode != 2 && (state.forceViewMode == 1 || bodyHighlighted || ropeHighlighted) && r.tension > 0.2f) {
            const Body& b = world.bodies[r.idB];
            Vector2 attachPt = worldToScreen(b.localToWorld(r.offsetB), sw, sh);
            float pushSign = (r.isSpring && r.signedExtension() < 0.0f) ? -1.0f : 1.0f;
            Vector2 endPt = {
                attachPt.x + r.dir.x * pushSign * r.tension * FORCE_SCALE,
                attachPt.y - r.dir.y * pushSign * r.tension * FORCE_SCALE
            };
            drawVectorArrow(attachPt, endPt, COL_GOLD,
                            TextFormat(r.isSpring ? "Fупр = %.1f Н" : "T = %.1f Н", r.tension), {10.0f, -8.0f});
        }

        if (ropeHighlighted) {
            Vector2 sA = worldToScreen(a.localToWorld(r.offsetA), sw, sh);
            Vector2 sB = worldToScreen(world.getRopeEndWorldPos(r), sw, sh);
            Vector2 mid = {(sA.x + sB.x) * 0.5f, (sA.y + sB.y) * 0.5f};
            drawRopeTooltip(r, mid);
        }
    }

    // 5. Левый верхний блок подсказок
    float hudY = 18.0f;
    const char* forceModeStr = (state.forceViewMode == 0) ? "[V] - силы (при наведении)"
                             : (state.forceViewMode == 1) ? "[V] - силы (все)"
                                                          : "[V] - силы (скрыты)";

    DrawTextEx(fontMain, state.paused ? "[Пробел] - пауза" : "[Пробел] - симуляция", {20, hudY}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, forceModeStr, {20, hudY + 22}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "[R] - сброс сцены", {20, hudY + 44}, 15, 1.0f, COL_TEXT_MUTED);

    // 6. Нижний док и правый инспектор
    drawLiquidDock(sh, state);
    drawInspector(world, state, sw, sh);

    EndDrawing();
}