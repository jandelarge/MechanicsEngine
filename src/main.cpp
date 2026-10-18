#include "raylib.h"
#include "physics/PhysicsWorld.hpp"
#include <cstdio>
#include <vector>
#include <cmath>

const float PPM = 70.0f;

Vector2 WorldToScreen(Vec2 wPos, int screenW, int screenH) {
    return {
        screenW * 0.5f + wPos.x * PPM,
        screenH * 0.85f - wPos.y * PPM
    };
}

Font LoadSystemRussianFont(int fontSize) {
    std::vector<int> codepoints;
    for (int i = 32; i <= 126; i++) codepoints.push_back(i);
    for (int i = 0x0400; i <= 0x04FF; i++) codepoints.push_back(i);

    const char* fontPath = "C:\\Windows\\Fonts\\arialbd.ttf";
    if (!FileExists(fontPath)) fontPath = "C:\\Windows\\Fonts\\arial.ttf";

    Font font = LoadFontEx(fontPath, fontSize, codepoints.data(), (int)codepoints.size());
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    return font;
}

void DrawHatchedSupport(Vector2 pos, float width, float height) {
    float x1 = pos.x - width * 0.5f;
    float x2 = pos.x + width * 0.5f;
    float y = pos.y - height * 0.5f;

    DrawLineEx({x1, y}, {x2, y}, 3.0f, RAYWHITE);
    for (float curX = x1 + 4.0f; curX <= x2; curX += 14.0f) {
        DrawLineEx({curX, y}, {curX - 10.0f, y - 10.0f}, 1.5f, LIGHTGRAY);
    }
}

void DrawVectorArrow(Vector2 start, Vector2 end, Color color, const char* label, Font font, Vector2 labelOffset = {8.0f, -8.0f}) {
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
    DrawTextEx(font, label, {end.x + labelOffset.x, end.y + labelOffset.y}, 15, 1.0f, color);
}

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 760, "Симулятор механики");
    SetTargetFPS(60);

    Font fontMain = LoadSystemRussianFont(18);
    Font fontBold = LoadSystemRussianFont(20);

    PhysicsWorld world;
    world.groundY = 0.5f;

    // Опора строго по центру экрана сверху
    int anchorId = world.createBody({0.0f, 7.5f}, 3.5f, 0.2f, 0.0f, true);

    // Подвешенный груз 5 кг (начинает чуть ниже опоры с учетом длины нити 3.0м)
    int hangingId = world.createBody({0.0f, 4.0f}, 1.3f, 1.3f, 5.0f, false);

    // Свободный груз 2 кг рядом на полу или чуть выше
    int freeId = world.createBody({-3.2f, 1.5f}, 1.0f, 1.0f, 2.0f, false);

    // Нить: от опоры до подвешенного груза, длина 3.0 м
    world.addRope(anchorId, hangingId, {0.0f, 0.0f}, {0.0f, 0.65f}, 3.0f, 220.0f);
    bool paused = false;
    bool showAllForces = false;
    int draggedId = -1;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_V)) showAllForces = !showAllForces;

        if (IsKeyPressed(KEY_R)) {
            world.bodies[hangingId].pos = {0.0f, 4.6f};
            world.bodies[hangingId].vel = {0.0f, 0.0f};
            world.bodies[freeId].pos = {-3.2f, 1.0f};
            world.bodies[freeId].vel = {0.0f, 0.0f};
        }

        Vector2 mouse = GetMousePosition();
        float wx = (mouse.x - GetScreenWidth() * 0.5f) / PPM;
        float wy = (GetScreenHeight() * 0.85f - mouse.y) / PPM;

        int hoveredId = -1;
        for (const auto& b : world.bodies) {
            if (!b.isStatic) {
                if (wx >= b.pos.x - b.width * 0.5f && wx <= b.pos.x + b.width * 0.5f &&
                    wy >= b.pos.y - b.height * 0.5f && wy <= b.pos.y + b.height * 0.5f) {
                    hoveredId = b.id;
                    break;
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoveredId != -1) {
            draggedId = hoveredId;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && draggedId != -1) {
            world.bodies[draggedId].pos = {wx, wy};
            world.bodies[draggedId].vel = {0.0f, 0.0f};
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            draggedId = -1;
        }

        if (!paused) {
            world.step(dt, draggedId);
        } else {
            world.updateForces();
        }

        BeginDrawing();
        ClearBackground({22, 24, 29, 255});

        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        float groundScrY = sh * 0.85f - world.groundY * PPM;
        DrawLine(0, (int)groundScrY, sw, (int)groundScrY, {70, 75, 85, 255});

        // 1. Отрисовка нити
        for (const auto& r : world.ropes) {
            const Body& a = world.bodies[r.idA];
            const Body& b = world.bodies[r.idB];

            Vector2 sA = WorldToScreen(a.pos + r.offsetA, sw, sh);
            Vector2 sB = WorldToScreen(b.pos + r.offsetB, sw, sh);

            Color ropeCol = (r.tension > 0.1f) ? Color{235, 175, 40, 255} : Color{110, 115, 125, 255};
            DrawLineEx(sA, sB, 3.0f, ropeCol);
        }

        // 2. Отрисовка тел
        for (const auto& b : world.bodies) {
            Vector2 scr = WorldToScreen(b.pos, sw, sh);
            float bw = b.width * PPM;
            float bh = b.height * PPM;

            if (b.isStatic) {
                DrawHatchedSupport(scr, bw, bh);
            } else {
                Rectangle rec = {scr.x - bw * 0.5f, scr.y - bh * 0.5f, bw, bh};
                bool isSelected = (b.id == hoveredId) || (b.id == draggedId);

                Color bodyCol = isSelected ? Color{41, 128, 185, 255} : Color{52, 73, 94, 255};
                DrawRectangleRec(rec, bodyCol);
                DrawRectangleLinesEx(rec, isSelected ? 2.5f : 1.5f, isSelected ? SKYBLUE : RAYWHITE);

                const char* massTxt = TextFormat("%.1f кг", b.mass);
                Vector2 txtSize = MeasureTextEx(fontBold, massTxt, 18, 1.0f);
                DrawTextEx(fontBold, massTxt, {scr.x - txtSize.x * 0.5f, scr.y - txtSize.y * 0.5f}, 18, 1.0f, WHITE);
            }
        }

        // 3. Отрисовка векторов сил
        const float FORCE_SCALE = 1.0f;

        for (const auto& b : world.bodies) {
            if (b.isStatic) continue;
            bool isSelected = (b.id == hoveredId) || (b.id == draggedId);
            if (!showAllForces && !isSelected) continue;

            Vector2 scr = WorldToScreen(b.pos, sw, sh);
            float bh = b.height * PPM;

            float fg = b.mass * world.G;
            DrawVectorArrow(scr, {scr.x, scr.y + fg * FORCE_SCALE}, {231, 76, 60, 255},
                            TextFormat("mg = %.1f Н", fg), fontMain, {10.0f, -8.0f});

            if (b.normalForce > 0.1f) {
                Vector2 bottomPt = {scr.x - 6.0f, scr.y + bh * 0.5f};
                DrawVectorArrow(bottomPt, {bottomPt.x, bottomPt.y - b.normalForce * FORCE_SCALE},
                                {46, 204, 113, 255}, TextFormat("N = %.1f Н", b.normalForce), fontMain, {-85.0f, -8.0f});
            }

            if (b.topLoadForce > 0.1f) {
                Vector2 topPt = {scr.x + 6.0f, scr.y - bh * 0.5f};
                DrawVectorArrow(topPt, {topPt.x, topPt.y + b.topLoadForce * FORCE_SCALE},
                                {230, 126, 34, 255}, TextFormat("P = %.1f Н", b.topLoadForce), fontMain, {10.0f, -8.0f});
            }
        }

        // Стрелка натяжения нити T и индикация удлинения dL
        for (const auto& r : world.ropes) {
            const Body& b = world.bodies[r.idB];
            bool isSelected = (b.id == hoveredId) || (b.id == draggedId);
            if ((showAllForces || isSelected) && r.tension > 0.1f) {
                Vector2 attachPt = WorldToScreen(b.pos + r.offsetB, sw, sh);
                Vector2 endPt = {
                    attachPt.x + r.dir.x * r.tension * FORCE_SCALE,
                    attachPt.y - r.dir.y * r.tension * FORCE_SCALE
                };
                float extCm = r.extension() * 100.0f;
                DrawVectorArrow(attachPt, endPt, {235, 175, 40, 255},
                                TextFormat("T = %.1f Н (+%.1f см)", r.tension, extCm), fontMain, {10.0f, -8.0f});
            }
        }

        DrawTextEx(fontBold, "Симулятор механики", {24, 20}, 20, 1.0f, RAYWHITE);
        DrawTextEx(fontMain, paused ? "Пауза [Пробел]" : "Симуляция [Пробел]", {24, 46}, 16, 1.0f,
                   paused ? Color{235, 175, 40, 255} : Color{46, 204, 113, 255});
        DrawTextEx(fontMain, showAllForces ? "Силы: Все [V]" : "Силы: При наведении [V]", {24, 68}, 15, 1.0f, SKYBLUE);
        DrawTextEx(fontMain, "ЛКМ - перемещать груз | R - сброс", {24, 90}, 14, 1.0f, LIGHTGRAY);

        EndDrawing();
    }

    UnloadFont(fontMain);
    UnloadFont(fontBold);
    CloseWindow();
    return 0;
}