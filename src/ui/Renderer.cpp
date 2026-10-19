#include "Renderer.hpp"
#include <vector>
#include <cmath>
#include <cstdio>

static Font LoadRussianFont(int fontSize) {
    std::vector<int> codepoints;
    for (int i = 32; i <= 126; i++) codepoints.push_back(i);
    for (int i = 0x0400; i <= 0x04FF; i++) codepoints.push_back(i);
    codepoints.push_back(0x0394); // Символ греческой дельты (Δ)

    const char* fontPath = "C:\\Windows\\Fonts\\arialbd.ttf";
    if (!FileExists(fontPath)) fontPath = "C:\\Windows\\Fonts\\arial.ttf";

    Font font = LoadFontEx(fontPath, fontSize, codepoints.data(), (int)codepoints.size());
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    return font;
}

void Renderer::init() {
    fontMain = LoadRussianFont(16);
    fontBold = LoadRussianFont(19);
}

void Renderer::unload() {
    UnloadFont(fontMain);
    UnloadFont(fontBold);
}

Vector2 Renderer::worldToScreen(Vec2 wPos, int sw, int sh) const {
    return {
        sw * 0.5f + wPos.x * ppm,
        sh * 0.85f - wPos.y * ppm
    };
}

void Renderer::drawHatchedSupport(Vector2 pos, float width, float height) const {
    float x1 = pos.x - width * 0.5f;
    float x2 = pos.x + width * 0.5f;
    float y = pos.y - height * 0.5f;

    DrawLineEx({x1, y}, {x2, y}, 3.0f, RAYWHITE);
    for (float curX = x1 + 4.0f; curX <= x2; curX += 14.0f) {
        DrawLineEx({curX, y}, {curX - 10.0f, y - 10.0f}, 1.5f, LIGHTGRAY);
    }
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
    float boxW = 245.0f;
    float boxH = 115.0f;

    DrawRectangle((int)boxX, (int)boxY, (int)boxW, (int)boxH, Fade({16, 18, 22, 255}, 0.92f));
    DrawRectangleLinesEx({boxX, boxY, boxW, boxH}, 1.5f, SKYBLUE);

    DrawTextEx(fontBold, "Параметры троса", {boxX + 12, boxY + 8}, 17, 1.0f, SKYBLUE);
    DrawTextEx(fontMain, TextFormat("Натяжение T: %.1f Н", r.tension), {boxX + 12, boxY + 32}, 15, 1.0f, LIGHTGRAY);
    DrawTextEx(fontMain, TextFormat("Растяжение \xCE\x94L: %.1f см", r.extension() * 100.0f), {boxX + 12, boxY + 52}, 15, 1.0f, LIGHTGRAY);
    DrawTextEx(fontMain, TextFormat("Длина L: %.2f м (L0: %.2f м)", r.currentLength, r.restLength), {boxX + 12, boxY + 72}, 15, 1.0f, LIGHTGRAY);
    DrawTextEx(fontMain, TextFormat("Жесткость k: %.0f Н/м", r.stiffness), {boxX + 12, boxY + 92}, 15, 1.0f, LIGHTGRAY);
}

void Renderer::draw(const PhysicsWorld& world, const AppState& state) {
    BeginDrawing();
    ClearBackground({22, 24, 29, 255});

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float groundScrY = sh * 0.85f - world.groundY * ppm;
    DrawLine(0, (int)groundScrY, sw, (int)groundScrY, {70, 75, 85, 255});

    // 1. Отрисовка тросов
    for (const auto& r : world.ropes) {
        const Body& a = world.bodies[r.idA];
        const Body& b = world.bodies[r.idB];

        Vector2 sA = worldToScreen(a.pos + r.offsetA, sw, sh);
        Vector2 sB = worldToScreen(b.pos + r.offsetB, sw, sh);

        bool isRopeHovered = (r.id == state.hoveredRopeId);
        Color ropeCol = isRopeHovered ? SKYBLUE : ((r.tension > 0.5f) ? Color{235, 175, 40, 255} : Color{110, 115, 125, 255});

        DrawLineEx(sA, sB, isRopeHovered ? 4.5f : 3.0f, ropeCol);
    }

    // 2. Отрисовка тел
    for (const auto& b : world.bodies) {
        Vector2 scr = worldToScreen(b.pos, sw, sh);
        float bw = b.width * ppm;
        float bh = b.height * ppm;

        if (b.isStatic) {
            drawHatchedSupport(scr, bw, bh);
        } else {
            Rectangle rec = {scr.x - bw * 0.5f, scr.y - bh * 0.5f, bw, bh};
            bool isSelected = (b.id == state.hoveredBodyId) || (b.id == state.draggedBodyId);

            Color bodyCol = isSelected ? Color{41, 128, 185, 255} : Color{52, 73, 94, 255};
            DrawRectangleRec(rec, bodyCol);
            DrawRectangleLinesEx(rec, isSelected ? 2.5f : 1.5f, isSelected ? SKYBLUE : RAYWHITE);

            const char* massTxt = TextFormat("%.1f кг", b.mass);
            Vector2 txtSize = MeasureTextEx(fontBold, massTxt, 18, 1.0f);
            DrawTextEx(fontBold, massTxt, {scr.x - txtSize.x * 0.5f, scr.y - txtSize.y * 0.5f}, 18, 1.0f, WHITE);
        }
    }

    // 3. Векторы сил выбранного тела
    const float FORCE_SCALE = 1.0f;

    for (const auto& b : world.bodies) {
        if (b.isStatic) continue;
        bool isSelected = (b.id == state.hoveredBodyId) || (b.id == state.draggedBodyId);
        if (!state.showAllForces && !isSelected) continue;

        Vector2 scr = worldToScreen(b.pos, sw, sh);
        float bh = b.height * ppm;

        float fg = b.mass * world.G;
        drawVectorArrow(scr, {scr.x, scr.y + fg * FORCE_SCALE}, {231, 76, 60, 255},
                        TextFormat("mg = %.1f Н", fg), {10.0f, -8.0f});

        if (b.normalForce > 0.1f) {
            Vector2 bottomPt = {scr.x - 6.0f, scr.y + bh * 0.5f};
            drawVectorArrow(bottomPt, {bottomPt.x, bottomPt.y - b.normalForce * FORCE_SCALE},
                            {46, 204, 113, 255}, TextFormat("N = %.1f Н", b.normalForce), {-90.0f, -8.0f});
        }

        if (b.topLoadForce > 0.1f) {
            Vector2 topPt = {scr.x + 6.0f, scr.y - bh * 0.5f};
            drawVectorArrow(topPt, {topPt.x, topPt.y + b.topLoadForce * FORCE_SCALE},
                            {230, 126, 34, 255}, TextFormat("P = %.1f Н", b.topLoadForce), {10.0f, -8.0f});
        }
    }

    // 4. Векторы натяжения и карточка троса
    for (const auto& r : world.ropes) {
        const Body& a = world.bodies[r.idA];
        const Body& b = world.bodies[r.idB];
        bool bodySelected = (b.id == state.hoveredBodyId) || (b.id == state.draggedBodyId);
        bool ropeSelected = (r.id == state.hoveredRopeId);

        if ((state.showAllForces || bodySelected || ropeSelected) && r.tension > 0.1f) {
            Vector2 attachPt = worldToScreen(b.pos + r.offsetB, sw, sh);
            Vector2 endPt = {
                attachPt.x + r.dir.x * r.tension * FORCE_SCALE,
                attachPt.y - r.dir.y * r.tension * FORCE_SCALE
            };
            drawVectorArrow(attachPt, endPt, {235, 175, 40, 255},
                            TextFormat("T = %.1f Н", r.tension), {10.0f, -8.0f});
        }

        if (ropeSelected) {
            Vector2 sA = worldToScreen(a.pos + r.offsetA, sw, sh);
            Vector2 sB = worldToScreen(b.pos + r.offsetB, sw, sh);
            Vector2 mid = {(sA.x + sB.x) * 0.5f, (sA.y + sB.y) * 0.5f};
            drawRopeTooltip(r, mid);
        }
    }

    // 5. Подсказки управления
    Color uiGray = LIGHTGRAY;
    DrawTextEx(fontMain, state.paused ? "[Пробел] - пауза" : "[Пробел] - симуляция", {24, 20}, 15, 1.0f, uiGray);
    DrawTextEx(fontMain, state.showAllForces ? "[V] - силы (все)" : "[V] - силы (при наведении)", {24, 42}, 15, 1.0f, uiGray);
    DrawTextEx(fontMain, "[ЛКМ] - перемещать груз", {24, 64}, 15, 1.0f, uiGray);
    DrawTextEx(fontMain, "[R] - сброс", {24, 86}, 15, 1.0f, uiGray);

    EndDrawing();
}