#include "Renderer.hpp"
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

static const Color COL_BG_DARK     = {25, 23, 22, 255};
static const Color COL_PANEL_BG    = {32, 29, 28, 255};
static const Color COL_WINE        = {119, 51, 68, 255};
static const Color COL_WINE_BRIGHT = {148, 66, 86, 255};
static const Color COL_WINE_DIM    = {82, 36, 48, 255};
static const Color COL_GOLD        = {194, 168, 120, 255};
static const Color COL_GOLD_DIM    = {132, 114, 84, 255};
static const Color COL_TEXT_CREAM  = {236, 228, 216, 255};
static const Color COL_TEXT_MUTED  = {166, 155, 140, 255};

struct LabMetaInfo {
    const char* title;
    const char* topic;
    const char* formula;
    const char* desc;
};

static const LabMetaInfo LAB_META[8] = {
    {"Лаб. 1: Движение по наклонной плоскости", "Динамика вращательного и поступательного движения",
     "a_шар = (2/3)*g*sin \xCE\xB1 | a_бр = g*(sin \xCE\xB1 - \xCE\xBC*cos \xCE\xB1)",
     "Сравнение ускорения катящегося сплошного шара (с учетом момента инерции I = 0.5*m*R\xC2\xB2) и скользящего бруска."},
    {"Лаб. 2: Гармонические колебания", "Закон Гука и сохранение механической энергии",
     "F_упр = -k*\xCE\x94x | T = 2*pi*sqrt(m / k)",
     "Исследование фазовых колебаний пружинных маятников разной жесткости k1 = 80 Н/м и k2 = 200 Н/м."},
    {"Лаб. 3: Закон сохранения импульса", "Абсолютно упругий центральный удар",
     "sum(m_i * v_i) = const | E_k1 = E_k2",
     "Передача импульса и кинетической энергии в цепочке упругих шаров после разгона ударника с горки."},
    {"Лаб. 4: Статика и предел силы трения", "Закон сухого трения Кулона-Амонтона",
     "F_тр.макс = \xCE\xBC * N = \xCE\xBC * (m1 + m2) * g",
     "Проверка условия покоя составного груза (5 кг) и равноускоренного движения одиночного бруска (2 кг) при тяге F = 18 Н."},
    {"Лаб. 5: Машина Атвуда (Неподвижный блок)", "Динамика связанных тел на блоке",
     "a = g * |m1 - m2| / (m1 + m2) | T = 2*m1*m2*g / (m1 + m2)",
     "Определение ускорения системы и силы натяжения невесомой нити для грузов m1 = 3.2 кг и m2 = 2.0 кг."},
    {"Лаб. 6: Затухающие колебания в среде", "Диссипация механической энергии",
     "F_сопр = -b * v | E_полн(t) = E_0 * exp(-\xCE\xB4*t)",
     "Наблюдение убывания амплитуды и полной механической энергии пружинного маятника при наличии сопротивления воздуха."},
    {"Лаб. 7: Свободное падение и опыт Галилея", "Кинематика равноускоренного движения",
     "v(h) = sqrt(2 * g * \xCE\x94h) | y(t) = y_0 - 0.5*g*t\xC2\xB2",
     "Доказательство независимости времени падения в вакууме от массы тела (1 кг и 8 кг) со стробоскопическим трассером."},
    {"Лаб. 8: Связанные маятники и резонанс", "Обмен энергией в системе со связью",
     "E_1(t) + E_2(t) + E_пруж(t) = const",
     "Исследование биений и периодической перекачки энергии между двумя маятниками, соединенными упругой пружиной."}
};

static Font LoadRussianFont(int fontSize) {
    std::vector<int> codepoints;
    for (int i = 32; i <= 126; i++) codepoints.push_back(i);
    for (int i = 0x0400; i <= 0x04FF; i++) codepoints.push_back(i);
    codepoints.push_back(0x0394);
    codepoints.push_back(0x03BC);
    codepoints.push_back(0x03C9);
    codepoints.push_back(0x03B1);
    codepoints.push_back(0x03B4);
    codepoints.push_back(0x00B2);
    codepoints.push_back(0x00B0);

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
    fontTitle = LoadRussianFont(28);
}

void Renderer::unload() {
    UnloadFont(fontSmall);
    UnloadFont(fontMain);
    UnloadFont(fontBold);
    UnloadFont(fontTitle);
}

Vector2 Renderer::worldToScreen(Vec2 wPos, int sw, int sh) const {
    float canvasWidth = (float)(sw - InputHandler::INSPECTOR_WIDTH);
    return {
        canvasWidth * 0.5f + (wPos.x - camPos.x) * ppm,
        sh * 0.5f - (wPos.y - camPos.y) * ppm
    };
}

void Renderer::drawMainMenu(int sw, int sh) const {
    for (int x = 0; x < sw; x += 60) DrawLine(x, 0, x, sh, {31, 28, 27, 255});
    for (int y = 0; y < sh; y += 60) DrawLine(0, y, sw, y, {31, 28, 27, 255});

    float cardW = 540.0f;
    float cardH = 470.0f;
    float cx = ((float)sw - cardW) * 0.5f;
    float cy = ((float)sh - cardH) * 0.5f;

    DrawRectangleRounded({cx + 4.0f, cy + 6.0f, cardW, cardH}, 0.06f, 12, {10, 8, 8, 180});
    DrawRectangleRounded({cx, cy, cardW, cardH}, 0.06f, 12, COL_PANEL_BG);
    DrawRectangleRoundedLinesEx({cx, cy, cardW, cardH}, 0.06f, 12, 1.8f, COL_GOLD);

    const char* title = "MECHANICS ENGINE";
    Vector2 tsT = MeasureTextEx(fontTitle, title, 28, 1.0f);
    DrawTextEx(fontTitle, title, {cx + (cardW - tsT.x) * 0.5f, cy + 28.0f}, 28, 1.0f, COL_GOLD);

    const char* sub = "Интерактивная цифровая физическая лаборатория v1.0.0";
    Vector2 tsS = MeasureTextEx(fontMain, sub, 15, 1.0f);
    DrawTextEx(fontMain, sub, {cx + (cardW - tsS.x) * 0.5f, cy + 64.0f}, 15, 1.0f, COL_TEXT_CREAM);

    DrawLineEx({cx + 40.0f, cy + 94.0f}, {cx + cardW - 40.0f, cy + 94.0f}, 1.2f, Fade(COL_GOLD, 0.35f));

    float menuW = 460.0f;
    float startX = ((float)sw - menuW) * 0.5f;
    float startY = (float)sh * 0.5f - 86.0f;

    const char* btnLabels[5] = {
        "Учебный центр (Лабораторные работы)",
        "Песочница (Свободное моделирование)",
        "Настройки физического ядра",
        "О проекте и архитектуре движка",
        "Выход из программы"
    };

    for (int i = 0; i < 5; ++i) {
        Rectangle r = {startX, startY + i * 60.0f, menuW, 48.0f};
        drawButton(r, btnLabels[i], i == 0 || i == 1, (i == 4) ? COL_WINE_BRIGHT : COL_GOLD);
    }

    const char* foot = "C++17 | Raylib 5.5 | SAT + Sutherland-Hodgman | 2x2 Block Impulse Solver";
    Vector2 tsF = MeasureTextEx(fontSmall, foot, 14, 1.0f);
    DrawTextEx(fontSmall, foot, {cx + (cardW - tsF.x) * 0.5f, cy + cardH - 26.0f}, 14, 1.0f, COL_TEXT_MUTED);
}

void Renderer::drawStudyHub(const PhysicsWorld& world, int sw, int sh) const {
    for (int x = 0; x < sw; x += 60) DrawLine(x, 0, x, sh, {31, 28, 27, 255});
    for (int y = 0; y < sh; y += 60) DrawLine(0, y, sw, y, {31, 28, 27, 255});

    drawButton({40.0f, 26.0f, 190.0f, 36.0f}, "<- В главное меню", false, COL_GOLD);
    drawButton({(float)sw - 210.0f, 26.0f, 170.0f, 36.0f}, "Настройки ядра", false, COL_GOLD);

    const char* hdr = "УЧЕБНЫЙ ЦЕНТР — КАТАЛОГ ЛАБОРАТОРНЫХ РАБОТ";
    Vector2 tsH = MeasureTextEx(fontTitle, hdr, 24, 1.0f);
    DrawTextEx(fontTitle, hdr, {((float)sw - tsH.x) * 0.5f, 30.0f}, 24, 1.0f, COL_GOLD);

    float gridW = std::min((float)sw - 80.0f, 1080.0f);
    float startX = ((float)sw - gridW) * 0.5f;
    float startY = 98.0f;
    float cardW = (gridW - 24.0f) * 0.5f;
    float cardH = std::clamp(((float)sh - 130.0f) / 4.0f - 12.0f, 108.0f, 134.0f);

    Vector2 m = GetMousePosition();
    for (int i = 0; i < 8; ++i) {
        int col = i % 2;
        int row = i / 2;
        Rectangle c = {startX + col * (cardW + 24.0f), startY + row * (cardH + 12.0f), cardW, cardH};
        bool hovered = (m.x >= c.x && m.x <= c.x + c.width && m.y >= c.y && m.y <= c.y + c.height);
        bool active = (world.activePreset == i + 1);

        DrawRectangleRounded({c.x + 2.0f, c.y + 4.0f, c.width, c.height}, 0.1f, 8, {12, 10, 10, 160});
        DrawRectangleRounded(c, 0.1f, 8, hovered ? Color{44, 34, 36, 255} : COL_PANEL_BG);
        DrawRectangleRoundedLinesEx(c, 0.1f, 8, (hovered || active) ? 2.0f : 1.2f,
                                    hovered ? COL_GOLD : (active ? COL_WINE_BRIGHT : Color{68, 58, 54, 255}));

        DrawTextEx(fontBold, LAB_META[i].title, {c.x + 16.0f, c.y + 10.0f}, 18, 1.0f, COL_GOLD);
        DrawTextEx(fontSmall, LAB_META[i].topic, {c.x + 16.0f, c.y + 32.0f}, 14, 1.0f, COL_TEXT_CREAM);

        Rectangle fBox = {c.x + 16.0f, c.y + 54.0f, c.width - 150.0f, 24.0f};
        DrawRectangleRounded(fBox, 0.25f, 6, {24, 21, 20, 255});
        DrawTextEx(fontSmall, LAB_META[i].formula, {fBox.x + 8.0f, fBox.y + 5.0f}, 14, 1.0f, COL_GOLD);

        DrawTextEx(fontSmall, LAB_META[i].desc, {c.x + 16.0f, c.y + 84.0f}, 14, 1.0f, COL_TEXT_MUTED);

        Rectangle openBtn = {c.x + c.width - 124.0f, c.y + 52.0f, 108.0f, 28.0f};
        drawButton(openBtn, "Открыть ->", hovered, COL_GOLD);
    }
}

void Renderer::drawSettingsScreen(const PhysicsWorld& world, const AppState& state, int sw, int sh) const {
    for (int x = 0; x < sw; x += 60) DrawLine(x, 0, x, sh, {31, 28, 27, 255});
    for (int y = 0; y < sh; y += 60) DrawLine(0, y, sw, y, {31, 28, 27, 255});

    drawButton({40.0f, 26.0f, 190.0f, 36.0f}, "<- Назад", false, COL_GOLD);

    const char* hdr = "НАСТРОЙКИ ФИЗИЧЕСКОГО ЯДРА И ОТОБРАЖЕНИЯ";
    Vector2 tsH = MeasureTextEx(fontTitle, hdr, 24, 1.0f);
    DrawTextEx(fontTitle, hdr, {((float)sw - tsH.x) * 0.5f, 30.0f}, 24, 1.0f, COL_GOLD);

    float panW = 680.0f;
    float panH = 480.0f;
    float px = ((float)sw - panW) * 0.5f;
    float py = 96.0f;
    float bx = px + 24.0f;
    float bw = panW - 48.0f;
    float halfW = (bw - 12.0f) * 0.5f;

    DrawRectangleRounded({px, py, panW, panH}, 0.05f, 10, COL_PANEL_BG);
    DrawRectangleRoundedLinesEx({px, py, panW, panH}, 0.05f, 10, 1.5f, COL_GOLD);

    DrawTextEx(fontBold, "1. Численный метод интегрирования уравнений движения:", {bx, py + 22.0f}, 18, 1.0f, COL_GOLD);
    drawButton({bx, py + 52.0f, bw, 36.0f},  "Симплектический метод Эйлера (Сохраняет фазовый объем и энергию)", world.integratorType == 0, COL_GOLD);
    drawButton({bx, py + 94.0f, bw, 36.0f},  "Скоростной метод Верле (2-й порядок точности траекторий)",         world.integratorType == 2, COL_GOLD);
    drawButton({bx, py + 136.0f, bw, 36.0f}, "Явный метод Эйлера (Демонстрация численного дрейфа энергии)",      world.integratorType == 1, COL_GOLD);

    DrawTextEx(fontBold, "2. Модель сопротивления внешней среды:", {bx, py + 184.0f}, 18, 1.0f, COL_GOLD);
    drawButton({bx, py + 214.0f, halfW, 36.0f},               "Идеальный вакуум (Без потерь)", !world.airResistance, COL_GOLD);
    drawButton({bx + halfW + 12.0f, py + 214.0f, halfW, 36.0f}, "Воздушная среда (Линейно-квадр.)", world.airResistance, COL_GOLD);

    DrawTextEx(fontBold, "3. Точность дискретизации шага решателя (Substeps):", {bx, py + 262.0f}, 18, 1.0f, COL_GOLD);
    float sw4 = (bw - 18.0f) / 4.0f;
    int subVals[4] = {12, 24, 48, 80};
    const char* subLbls[4] = {"12 шагов", "24 шага (Станд.)", "48 шагов (Высок.)", "80 шагов (Эталон)"};
    for (int i = 0; i < 4; ++i) {
        drawButton({bx + i * (sw4 + 6.0f), py + 292.0f, sw4, 36.0f}, subLbls[i], world.substeps == subVals[i], COL_GOLD);
    }

    DrawTextEx(fontBold, "4. Инженерная телеметрия и диагностика:", {bx, py + 340.0f}, 18, 1.0f, COL_GOLD);
    drawButton({bx, py + 370.0f, halfW, 36.0f},               state.showCenterOfMass ? "Центр масс системы: Вкл" : "Центр масс системы: Выкл", state.showCenterOfMass, COL_GOLD);
    drawButton({bx + halfW + 12.0f, py + 370.0f, halfW, 36.0f}, state.showProfiler ? "IT-Профайлер ядра: Вкл" : "IT-Профайлер ядра: Выкл", state.showProfiler, COL_GOLD);

    const char* fmStr = (state.forceViewMode == 0) ? "Режим векторов сил: При наведении курсора"
                      : (state.forceViewMode == 1) ? "Режим векторов сил: Показать все векторы"
                                                   : "Режим векторов сил: Скрыть векторы";
    drawButton({bx, py + 414.0f, bw, 36.0f}, fmStr, state.forceViewMode == 1, COL_GOLD);
}

void Renderer::drawAboutScreen(int sw, int sh) const {
    for (int x = 0; x < sw; x += 60) DrawLine(x, 0, x, sh, {31, 28, 27, 255});
    for (int y = 0; y < sh; y += 60) DrawLine(0, y, sw, y, {31, 28, 27, 255});

    drawButton({40.0f, 26.0f, 190.0f, 36.0f}, "<- Назад", false, COL_GOLD);

    const char* hdr = "О ПРОЕКТЕ И АРХИТЕКТУРЕ MECHANICS ENGINE";
    Vector2 tsH = MeasureTextEx(fontTitle, hdr, 24, 1.0f);
    DrawTextEx(fontTitle, hdr, {((float)sw - tsH.x) * 0.5f, 30.0f}, 24, 1.0f, COL_GOLD);

    float panW = 780.0f;
    float panH = 470.0f;
    float px = ((float)sw - panW) * 0.5f;
    float py = 96.0f;
    float tx = px + 28.0f;

    DrawRectangleRounded({px, py, panW, panH}, 0.05f, 10, COL_PANEL_BG);
    DrawRectangleRoundedLinesEx({px, py, panW, panH}, 0.05f, 10, 1.5f, COL_GOLD);

    DrawTextEx(fontBold, "Назначение программного комплекса:", {tx, py + 24.0f}, 18, 1.0f, COL_GOLD);
    DrawTextEx(fontMain, "MechanicsEngine — интерактивная среда физического моделирования и виртуальная лаборатория,", {tx, py + 50.0f}, 15, 1.0f, COL_TEXT_CREAM);
    DrawTextEx(fontMain, "разработанная на C++17 с собственным физическим ядром без использования сторонних движков.", {tx, py + 72.0f}, 15, 1.0f, COL_TEXT_CREAM);

    DrawTextEx(fontBold, "Ключевые алгоритмы физического решателя:", {tx, py + 112.0f}, 18, 1.0f, COL_GOLD);
    DrawTextEx(fontMain, "• Двухфазная детекция столкновений: Broad-Phase (AABB-отсечение) + Narrow-Phase (SAT).", {tx, py + 138.0f}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "• Генерация контактного многообразия методом клиппинга Сазерленда-Ходжмана.", {tx, py + 162.0f}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "• Импульсный решатель Sequential Impulses с блочным решателем СЛАУ 2x2 (Block Solver).", {tx, py + 186.0f}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "• Связанные неголономные и голономные связи: пружины Гука, стержни и блоки Атвуда.", {tx, py + 210.0f}, 15, 1.0f, COL_TEXT_MUTED);

    DrawTextEx(fontBold, "Инструменты верификации и анализа данных:", {tx, py + 250.0f}, 18, 1.0f, COL_GOLD);
    DrawTextEx(fontMain, "• Встроенный осциллограф реального времени с автоматическим расчетом погрешности модели.", {tx, py + 276.0f}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "• Стробоскопическое трассирование траекторий с засечками времени каждые 0.10 с.", {tx, py + 300.0f}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "• Экспорт телеметрии в формат CSV (UTF-8 BOM) и сериализация сцен в файлы .mec.", {tx, py + 324.0f}, 15, 1.0f, COL_TEXT_MUTED);

    DrawTextEx(fontBold, "Точность верификации:", {tx, py + 366.0f}, 18, 1.0f, COL_GOLD);
    DrawTextEx(fontMain, "Относительная погрешность численного расчета относительно аналитических формул", {tx, py + 392.0f}, 15, 1.0f, COL_TEXT_CREAM);
    DrawTextEx(fontMain, "классической механики составляет менее 0.25% при стандартном шаге дискретизации.", {tx, py + 414.0f}, 15, 1.0f, COL_TEXT_CREAM);
}

void Renderer::drawInfiniteGridAndFloor(const PhysicsWorld& world, int sw, int sh) const {
    int canvasW = sw - InputHandler::INSPECTOR_WIDTH;
    float leftW   = camPos.x - (canvasW * 0.5f) / ppm;
    float rightW  = camPos.x + (canvasW * 0.5f) / ppm;
    float topW    = camPos.y + (sh * 0.5f) / ppm;

    int gridStep = (ppm < 28.0f) ? 5 : 1;
    int startX = (int)std::floor(leftW / gridStep) * gridStep;
    int endX   = (int)std::ceil(rightW / gridStep) * gridStep;

    for (int x = startX; x <= endX; x += gridStep) {
        Vector2 pTop = worldToScreen({(float)x, topW}, sw, sh);
        Vector2 pBot = worldToScreen({(float)x, world.groundY}, sw, sh);
        bool isMajor = (x % 5 == 0);
        DrawLineV(pTop, pBot, isMajor ? Color{40, 36, 34, 255} : Color{31, 28, 27, 255});
    }

    int maxStepsY = (int)std::ceil((topW - world.groundY) / gridStep);
    for (int k = gridStep; k <= maxStepsY * gridStep; k += gridStep) {
        float y = world.groundY + (float)k;
        if (y > topW) break;
        Vector2 pL = worldToScreen({leftW, y}, sw, sh);
        Vector2 pR = worldToScreen({rightW, y}, sw, sh);
        bool isMajor = (k % 5 == 0);
        DrawLineV(pL, pR, isMajor ? Color{40, 36, 34, 255} : Color{31, 28, 27, 255});
    }

    Vector2 gL = worldToScreen({leftW, world.groundY}, sw, sh);
    Vector2 gR = worldToScreen({rightW, world.groundY}, sw, sh);
    DrawLineEx(gL, gR, 2.0f, COL_GOLD_DIM);

    for (int x = startX; x <= endX; x += gridStep) {
        Vector2 pt = worldToScreen({(float)x, world.groundY}, sw, sh);
        bool isMajor = (x % 5 == 0);
        DrawLineEx(pt, {pt.x, pt.y + (isMajor ? 8.0f : 4.0f)}, isMajor ? 1.8f : 1.0f, COL_GOLD_DIM);
        if (isMajor) {
            const char* lbl = TextFormat("%d м", x);
            Vector2 ts = MeasureTextEx(fontSmall, lbl, 14, 1.0f);
            DrawTextEx(fontSmall, lbl, {pt.x - ts.x * 0.5f, pt.y + 10.0f}, 14, 1.0f, Fade(COL_TEXT_MUTED, 0.75f));
        }
    }
}

void Renderer::drawTrails(const PhysicsWorld& world, const AppState& state, int sw, int sh) const {
    for (const auto& b : world.bodies) {
        if (b.isStatic || b.trailMode == 0 || b.trail.size() < 2) continue;

        bool isBodyHidden = state.hideAllBodies || b.hidden;
        bool isSelected   = (b.id == state.selectedBodyId) || (b.id == state.hoveredBodyId);

        Color lineCol = (isBodyHidden || isSelected) ? Fade(COL_GOLD, 0.90f) : Fade(COL_GOLD, 0.55f);
        float thick   = isBodyHidden ? 2.4f : 1.8f;

        for (size_t i = 1; i < b.trail.size(); ++i) {
            Vector2 p0 = worldToScreen(b.trail[i - 1].pos, sw, sh);
            Vector2 p1 = worldToScreen(b.trail[i].pos, sw, sh);
            DrawLineEx(p0, p1, thick, lineCol);
        }

        for (size_t i = 0; i < b.trail.size(); ++i) {
            if (b.trail[i].strobe) {
                Vector2 pt = worldToScreen(b.trail[i].pos, sw, sh);
                DrawCircleV(pt, isBodyHidden ? 3.6f : 2.8f, isBodyHidden ? COL_TEXT_CREAM : COL_GOLD);
            }
        }

        if (isBodyHidden) {
            Vector2 cur = worldToScreen(b.pos, sw, sh);
            DrawCircleV(cur, 5.0f, COL_WINE_BRIGHT);
            DrawCircleLines((int)cur.x, (int)cur.y, 5.0f, COL_GOLD);
        }
    }
}

void Renderer::drawPulleys(const PhysicsWorld& world, const AppState& state, int sw, int sh) const {
    if (state.hideAllBodies) return;
    const float FORCE_SCALE = 0.9f;

    for (const auto& p : world.pulleys) {
        if (p.idA < 0 || p.idB < 0 || p.idA >= (int)world.bodies.size() || p.idB >= (int)world.bodies.size()) continue;

        Vector2 wScr = worldToScreen(p.wheelPos, sw, sh);
        float rPx = p.radius * ppm;

        Vector2 mountTop = worldToScreen({p.wheelPos.x, p.wheelPos.y + p.radius + 0.35f}, sw, sh);
        DrawLineEx(mountTop, wScr, 4.0f, COL_GOLD_DIM);
        DrawCircleV(mountTop, 4.5f, COL_GOLD);

        DrawCircleV(wScr, rPx, {36, 31, 30, 255});
        DrawRing(wScr, rPx - 3.0f, rPx, 0.0f, 360.0f, 48, COL_GOLD);

        for (int k = 0; k < 4; ++k) {
            float ang = p.wheelAngle + k * 0.785398f;
            Vec2 d = {std::cos(ang), std::sin(ang)};
            Vector2 s1 = worldToScreen(p.wheelPos - d * (p.radius * 0.85f), sw, sh);
            Vector2 s2 = worldToScreen(p.wheelPos + d * (p.radius * 0.85f), sw, sh);
            DrawLineEx(s1, s2, 1.6f, Fade(COL_GOLD, 0.45f));
        }
        DrawCircleV(wScr, 5.0f, COL_WINE_BRIGHT);
        DrawCircleLines((int)wScr.x, (int)wScr.y, 5.0f, COL_GOLD);

        Vector2 ancL = worldToScreen(p.anchorLeft(), sw, sh);
        Vector2 ancR = worldToScreen(p.anchorRight(), sw, sh);
        Vector2 pA = worldToScreen(world.bodies[p.idA].localToWorld(p.offsetA), sw, sh);
        Vector2 pB = worldToScreen(world.bodies[p.idB].localToWorld(p.offsetB), sw, sh);

        DrawLineEx(ancL, pA, 3.0f, COL_GOLD);
        DrawLineEx(ancR, pB, 3.0f, COL_GOLD);
        DrawCircleV(pA, 3.8f, COL_TEXT_CREAM);
        DrawCircleV(pB, 3.8f, COL_TEXT_CREAM);

        if (state.forceViewMode != 2 && p.tension > 0.2f) {
            bool showA = (state.forceViewMode == 1) || (state.hoveredBodyId == p.idA) || (state.selectedBodyId == p.idA);
            bool showB = (state.forceViewMode == 1) || (state.hoveredBodyId == p.idB) || (state.selectedBodyId == p.idB);
            if (showA) {
                Vector2 endA = {pA.x + p.dirA.x * p.tension * FORCE_SCALE, pA.y - p.dirA.y * p.tension * FORCE_SCALE};
                drawVectorArrow(pA, endA, COL_GOLD, TextFormat("T = %.1f Н", p.tension), {-85.0f, -8.0f});
            }
            if (showB) {
                Vector2 endB = {pB.x + p.dirB.x * p.tension * FORCE_SCALE, pB.y - p.dirB.y * p.tension * FORCE_SCALE};
                drawVectorArrow(pB, endB, COL_GOLD, TextFormat("T = %.1f Н", p.tension), {10.0f, -8.0f});
            }
        }
    }
}

void Renderer::drawCenterOfMass(const PhysicsWorld& world, const AppState& state, int sw, int sh) const {
    if (!state.showCenterOfMass) return;

    Vec2 cmPos, cmVel;
    float totalM = 0.0f;
    if (!world.computeCenterOfMass(cmPos, cmVel, totalM)) return;

    Vector2 scr = worldToScreen(cmPos, sw, sh);
    Color cmCol = {96, 210, 224, 255};

    DrawCircleV(scr, 6.5f, Fade(COL_BG_DARK, 0.85f));
    DrawCircleLines((int)scr.x, (int)scr.y, 6.5f, cmCol);
    DrawLineEx({scr.x - 11.0f, scr.y}, {scr.x + 11.0f, scr.y}, 2.0f, cmCol);
    DrawLineEx({scr.x, scr.y - 11.0f}, {scr.x, scr.y + 11.0f}, 2.0f, cmCol);

    if (cmVel.length() > 0.05f) {
        Vector2 vEnd = {scr.x + cmVel.x * 22.0f, scr.y - cmVel.y * 22.0f};
        drawVectorArrow(scr, vEnd, cmCol, TextFormat("Vцм = %.2f м/с", cmVel.length()), {8.0f, -18.0f});
    }

    const char* lbl = TextFormat("ЦМ (M = %.1f кг | P = %.1f кг*м/с)", totalM, totalM * cmVel.length());
    DrawTextEx(fontSmall, lbl, {scr.x + 10.0f, scr.y + 8.0f}, 14, 1.0f, cmCol);
}

void Renderer::drawProfilerOverlay(const PhysicsWorld& world, const AppState& state) const {
    if (!state.showProfiler) return;

    Rectangle box = {20.0f, 160.0f, 248.0f, 118.0f};
    DrawRectangleRounded(box, 0.14f, 8, {26, 22, 21, 235});
    DrawRectangleRoundedLinesEx(box, 0.14f, 8, 1.4f, COL_GOLD);

    const char* intName = (world.integratorType == 0) ? "Симпл. Эйлер"
                        : (world.integratorType == 1) ? "Явный Эйлер" : "Метод Верле";

    DrawTextEx(fontBold, "IT-Профайлер ядра", {box.x + 10.0f, box.y + 6.0f}, 16, 1.0f, COL_GOLD);
    DrawTextEx(fontSmall, TextFormat("FPS: %d | Шаг ядра: %.3f мс", GetFPS(), world.lastStepTimeMs), {box.x + 10.0f, box.y + 28.0f}, 14, 1.0f, COL_TEXT_CREAM);
    DrawTextEx(fontSmall, TextFormat("Broad-Phase (AABB): %d пар", world.broadphasePairs), {box.x + 10.0f, box.y + 46.0f}, 14, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontSmall, TextFormat("Narrow-Phase (SAT): %d тестов", world.narrowphaseTests), {box.x + 10.0f, box.y + 64.0f}, 14, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontSmall, TextFormat("Активных контактов: %d (Sub: %d)", world.activeContactsCount, world.substeps), {box.x + 10.0f, box.y + 82.0f}, 14, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontSmall, TextFormat("Решатель: %s | %s", intName, world.airResistance ? "Воздух" : "Вакуум"), {box.x + 10.0f, box.y + 100.0f}, 14, 1.0f, COL_GOLD);
}

void Renderer::drawChronoBar(const PhysicsWorld& world, const AppState& state, int sw) const {
    Rectangle bar = InputHandler::getChronoBarRect(sw);

    DrawRectangleRounded({bar.x + 2.0f, bar.y + 4.0f, bar.width, bar.height}, 0.45f, 12, {10, 8, 8, 150});
    DrawRectangleRounded(bar, 0.45f, 12, {34, 29, 28, 238});
    DrawRectangleRoundedLinesEx(bar, 0.45f, 12, 1.4f, Fade(COL_GOLD, 0.45f));

    float x = bar.x + 8.0f;
    float y = bar.y + 6.0f;
    float h = 28.0f;

    Rectangle timeBox = {x, y, 104.0f, h};
    DrawRectangleRounded(timeBox, 0.3f, 6, {24, 21, 20, 255});
    DrawRectangleRoundedLinesEx(timeBox, 0.3f, 6, 1.0f, {68, 58, 54, 255});
    const char* tStr = TextFormat("t = %.2f с", world.simTime);
    Vector2 ts = MeasureTextEx(fontMain, tStr, 15, 1.0f);
    DrawTextEx(fontMain, tStr, {timeBox.x + (timeBox.width - ts.x) * 0.5f, timeBox.y + (h - ts.y) * 0.5f},
               15, 1.0f, state.paused ? COL_GOLD : COL_TEXT_CREAM);
    x += 110.0f;

    drawButton({x, y, 68.0f, h}, "Сброс t", false, COL_GOLD);
    x += 74.0f;

    drawButton({x, y, 68.0f, h}, ">| Шаг", false, COL_GOLD);
    x += 74.0f;

    DrawLineEx({x, bar.y + 9.0f}, {x, bar.y + bar.height - 9.0f}, 1.5f, Fade(COL_GOLD, 0.25f));
    x += 6.0f;

    float scales[3] = {0.25f, 0.5f, 1.0f};
    const char* sLabels[3] = {"0.25x", "0.5x", "1.0x"};
    for (int i = 0; i < 3; ++i) {
        bool active = (std::abs(state.timeScale - scales[i]) < 0.03f);
        drawButton({x, y, 48.0f, h}, sLabels[i], active, COL_GOLD);
        x += 52.0f;
    }

    if (state.statusTimer > 0.0f && !state.statusMessage.empty()) {
        Vector2 sts = MeasureTextEx(fontMain, state.statusMessage.c_str(), 15, 1.0f);
        Rectangle toast = {bar.x + (bar.width - sts.x - 28.0f) * 0.5f, bar.y + bar.height + 8.0f, sts.x + 28.0f, 28.0f};
        DrawRectangleRounded(toast, 0.4f, 8, {32, 46, 34, 240});
        DrawRectangleRoundedLinesEx(toast, 0.4f, 8, 1.5f, {126, 186, 132, 255});
        DrawTextEx(fontMain, state.statusMessage.c_str(), {toast.x + 14.0f, toast.y + 6.0f}, 15, 1.0f, COL_TEXT_CREAM);
    }
}

void Renderer::drawOscilloscope(const PhysicsWorld& world, const AppState& state, int sw, int sh) const {
    if (!state.showGraph) return;

    Rectangle gr = InputHandler::getGraphRect(state, sw, sh);
    DrawRectangleRounded({gr.x + 3.0f, gr.y + 5.0f, gr.width, gr.height}, 0.06f, 10, {10, 8, 8, 185});
    DrawRectangleRounded(gr, 0.06f, 10, {30, 26, 25, 248});

    DrawRectangleRounded({gr.x, gr.y, gr.width, 34.0f}, 0.16f, 8, {38, 32, 31, 255});
    DrawLineEx({gr.x + 1.0f, gr.y + 34.0f}, {gr.x + gr.width - 1.0f, gr.y + 34.0f}, 1.0f, {62, 52, 48, 255});
    DrawRectangleRoundedLinesEx(gr, 0.06f, 10, (state.isDraggingGraph || state.isResizingGraph) ? 2.0f : 1.5f, COL_GOLD);

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 2; ++c) {
            DrawCircleV({gr.x + 10.0f + c * 4.0f, gr.y + 12.0f + r * 5.0f}, 1.2f, COL_GOLD_DIM);
        }
    }

    DrawTextEx(fontBold, "Осциллограф", {gr.x + 22.0f, gr.y + 9.0f}, 17, 1.0f, COL_GOLD);

    drawButton({gr.x + gr.width - 332.0f, gr.y + 6.0f, 72.0f, 24.0f}, "Энергия",     state.graphTab == 0, COL_GOLD);
    drawButton({gr.x + gr.width - 256.0f, gr.y + 6.0f, 92.0f, 24.0f}, "Скор./Силы",  state.graphTab == 1, COL_GOLD);
    drawButton({gr.x + gr.width - 160.0f, gr.y + 6.0f, 96.0f, 24.0f}, "Верификация", state.graphTab == 2, COL_GOLD);
    drawButton({gr.x + gr.width - 60.0f,  gr.y + 6.0f, 32.0f, 24.0f}, "CSV",         false, {126, 186, 132, 255});
    drawButton({gr.x + gr.width - 24.0f,  gr.y + 6.0f, 20.0f, 24.0f}, "X",           false, COL_WINE_BRIGHT);

    Rectangle plot = {gr.x + 12.0f, gr.y + 40.0f, gr.width - 24.0f, gr.height - 76.0f};
    DrawRectangleRec(plot, {22, 19, 18, 255});
    DrawRectangleLinesEx(plot, 1.0f, {58, 50, 46, 255});

    for (int i = 1; i <= 3; ++i) {
        float ly = plot.y + (plot.height * i) / 4.0f;
        DrawLineV({plot.x, ly}, {plot.x + plot.width, ly}, {36, 32, 30, 255});
    }
    for (int i = 1; i <= 4; ++i) {
        float lx = plot.x + (plot.width * i) / 5.0f;
        DrawLineV({lx, plot.y}, {lx, plot.y + plot.height}, {36, 32, 30, 255});
    }

    const auto& hist = state.telemetryHistory;
    if (hist.size() >= 2) {
        float maxVal = 1.0f;
        for (const auto& s : hist) {
            if (state.graphTab == 0) {
                maxVal = std::max({maxVal, s.ek, s.ep, s.etot});
            } else if (state.graphTab == 1) {
                maxVal = std::max({maxVal, s.speed, s.normalF * 0.2f, s.frictionF * 0.2f});
            } else {
                maxVal = std::max({maxVal, s.theoryVal, s.simVal});
            }
        }
        maxVal *= 1.12f;

        auto drawCurve = [&](auto getter, Color col, float scaleMult) {
            for (size_t i = 1; i < hist.size(); ++i) {
                float x0 = plot.x + ((float)(i - 1) / (float)(hist.size() - 1)) * plot.width;
                float x1 = plot.x + ((float)i / (float)(hist.size() - 1)) * plot.width;
                float v0 = std::clamp(getter(hist[i - 1]) * scaleMult / maxVal, 0.0f, 1.0f);
                float v1 = std::clamp(getter(hist[i]) * scaleMult / maxVal, 0.0f, 1.0f);
                Vector2 p0 = {x0, plot.y + plot.height - v0 * plot.height};
                Vector2 p1 = {x1, plot.y + plot.height - v1 * plot.height};
                DrawLineEx(p0, p1, 2.0f, col);
            }
        };

        if (state.graphTab == 0) {
            drawCurve([](const TelemetrySample& s) { return s.etot; }, {126, 186, 132, 255}, 1.0f);
            drawCurve([](const TelemetrySample& s) { return s.ep; },   COL_GOLD, 1.0f);
            drawCurve([](const TelemetrySample& s) { return s.ek; },   {96, 178, 224, 255}, 1.0f);
        } else if (state.graphTab == 1) {
            drawCurve([](const TelemetrySample& s) { return s.normalF; },   {126, 186, 132, 255}, 0.2f);
            drawCurve([](const TelemetrySample& s) { return s.frictionF; }, {188, 122, 214, 255}, 0.2f);
            drawCurve([](const TelemetrySample& s) { return s.speed; },     {214, 92, 92, 255}, 1.0f);
        } else {
            drawCurve([](const TelemetrySample& s) { return s.theoryVal; }, COL_GOLD, 1.0f);
            drawCurve([](const TelemetrySample& s) { return s.simVal; },    {96, 178, 224, 255}, 1.0f);
        }
    } else {
        const char* hint = "Запустите симуляцию [Пробел] или [>| Шаг]";
        Vector2 hs = MeasureTextEx(fontSmall, hint, 14, 1.0f);
        DrawTextEx(fontSmall, hint, {plot.x + (plot.width - hs.x) * 0.5f, plot.y + (plot.height - hs.y) * 0.5f},
                   14, 1.0f, COL_TEXT_MUTED);
    }

    if (state.graphTab == 2) {
        const char* formulaStr = "Модель: Eполн(t) = Ek + Ep = const";
        if (!world.pulleys.empty()) formulaStr = "Модель Атвуда: a = g*|m1-m2|/(m1+m2)";
        else if (world.activePreset >= 1 && world.activePreset <= 8) formulaStr = LAB_META[world.activePreset - 1].formula;
        DrawTextEx(fontSmall, formulaStr, {plot.x + 8.0f, plot.y + 6.0f}, 14, 1.0f, COL_TEXT_CREAM);
    }

    TelemetrySample cur = hist.empty() ? TelemetrySample{} : hist.back();
    float legY = gr.y + gr.height - 26.0f;
    float colStep = (gr.width - 36.0f) / 3.0f;

    if (state.graphTab == 0) {
        DrawTextEx(fontSmall, TextFormat("Ek: %.1f Дж", cur.ek),      {gr.x + 14.0f,               legY}, 14, 1.0f, {96, 178, 224, 255});
        DrawTextEx(fontSmall, TextFormat("Ep: %.1f Дж", cur.ep),      {gr.x + 14.0f + colStep,     legY}, 14, 1.0f, COL_GOLD);
        DrawTextEx(fontSmall, TextFormat("Eполн: %.1f Дж", cur.etot), {gr.x + 14.0f + colStep * 2, legY}, 14, 1.0f, {126, 186, 132, 255});
    } else if (state.graphTab == 1) {
        DrawTextEx(fontSmall, TextFormat("|v|: %.2f м/с", cur.speed),   {gr.x + 14.0f,               legY}, 14, 1.0f, {214, 92, 92, 255});
        DrawTextEx(fontSmall, TextFormat("N: %.1f Н", cur.normalF),     {gr.x + 14.0f + colStep,     legY}, 14, 1.0f, {126, 186, 132, 255});
        DrawTextEx(fontSmall, TextFormat("Fтр: %.1f Н", cur.frictionF), {gr.x + 14.0f + colStep * 2, legY}, 14, 1.0f, {188, 122, 214, 255});
    } else {
        DrawTextEx(fontSmall, TextFormat("Теория: %.2f", cur.theoryVal),  {gr.x + 14.0f,               legY}, 14, 1.0f, COL_GOLD);
        DrawTextEx(fontSmall, TextFormat("Движок: %.2f", cur.simVal),     {gr.x + 14.0f + colStep,     legY}, 14, 1.0f, {96, 178, 224, 255});
        Color errCol = (cur.errorPct < 2.0f) ? Color{126, 186, 132, 255} : Color{214, 92, 92, 255};
        DrawTextEx(fontSmall, TextFormat("Погр. \xCE\xB4: %.2f%%", cur.errorPct), {gr.x + 14.0f + colStep * 2, legY}, 14, 1.0f, errCol);
    }

    Color gripCol = state.isResizingGraph ? COL_GOLD : COL_GOLD_DIM;
    float rx = gr.x + gr.width - 4.0f;
    float ry = gr.y + gr.height - 4.0f;
    DrawLineEx({rx - 11.0f, ry}, {rx, ry - 11.0f}, 1.5f, gripCol);
    DrawLineEx({rx - 7.0f,  ry}, {rx, ry - 7.0f},  1.5f, gripCol);
    DrawLineEx({rx - 3.0f,  ry}, {rx, ry - 3.0f},  1.5f, gripCol);
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
        case 0: {
            Rectangle box = {cx - 7.5f, cy - 7.5f, 15.0f, 15.0f};
            DrawRectangleRounded(box, 0.2f, 4, Fade(col, active ? 0.35f : 0.15f));
            DrawRectangleRoundedLinesEx(box, 0.2f, 4, 1.8f, col);
            DrawCircleV({cx, cy}, 1.8f, col);
            break;
        }
        case 1: {
            DrawCircleV({cx, cy}, 8.0f, Fade(col, active ? 0.35f : 0.15f));
            DrawCircleLines((int)cx, (int)cy, 8.0f, col);
            DrawLineEx({cx, cy}, {cx + 5.8f, cy - 5.2f}, 1.8f, col);
            DrawCircleV({cx, cy}, 1.8f, col);
            break;
        }
        case 2: {
            Rectangle sup = {cx - 9.5f, cy - 3.5f, 19.0f, 7.0f};
            DrawRectangleRounded(sup, 0.8f, 6, Fade(col, active ? 0.28f : 0.12f));
            DrawRectangleRoundedLinesEx(sup, 0.8f, 6, 1.6f, col);
            break;
        }
        case 3: {
            Vector2 a = {cx - 6.0f, cy - 6.0f};
            Vector2 b = {cx + 6.0f, cy + 6.0f};
            DrawLineEx(a, b, 2.0f, col);
            DrawCircleV(a, 3.0f, COL_BG_DARK);
            DrawCircleLines((int)a.x, (int)a.y, 3.0f, col);
            DrawCircleV(b, 3.0f, COL_BG_DARK);
            DrawCircleLines((int)b.x, (int)b.y, 3.0f, col);
            break;
        }
        case 4: {
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
        case 5: {
            DrawCircleLines((int)cx, (int)(cy - 2.0f), 5.5f, col);
            DrawCircleV({cx, cy - 2.0f}, 1.8f, col);
            DrawLineEx({cx - 5.5f, cy - 2.0f}, {cx - 5.5f, cy + 7.5f}, 1.6f, col);
            DrawLineEx({cx + 5.5f, cy - 2.0f}, {cx + 5.5f, cy + 7.5f}, 1.6f, col);
            break;
        }
    }
}

void Renderer::drawDockIcon(int index, Rectangle r, bool active, bool paused, Color col) const {
    float cx = r.x + r.width * 0.5f;
    float cy = r.y + r.height * 0.5f;

    switch (index) {
        case 0: {
            if (paused) {
                DrawTriangle({cx - 5.0f, cy - 8.0f}, {cx - 5.0f, cy + 8.0f}, {cx + 8.0f, cy}, col);
            } else {
                DrawRectangleRec({cx - 6.0f, cy - 8.0f, 4.0f, 16.0f}, col);
                DrawRectangleRec({cx + 2.0f, cy - 8.0f, 4.0f, 16.0f}, col);
            }
            break;
        }
        case 1: {
            DrawRing({cx, cy}, 6.4f, 8.8f, 42.0f, 312.0f, 28, col);
            Vector2 tip       = {cx + 8.6f, cy - 2.4f};
            Vector2 outerBase = {cx + 7.2f, cy - 9.4f};
            Vector2 innerBase = {cx + 1.8f, cy - 3.2f};
            DrawTriangle(tip, outerBase, innerBase, col);
            break;
        }
        case 2: {
            Vector2 p1 = {cx - 6.0f, cy - 10.0f};
            Vector2 p2 = {cx - 6.0f, cy + 7.0f};
            Vector2 p3 = {cx + 6.0f, cy + 1.0f};
            DrawTriangle(p1, p2, p3, col);
            DrawLineEx({cx - 2.0f, cy + 2.0f}, {cx + 3.0f, cy + 9.0f}, 2.5f, col);
            break;
        }
        case 3: {
            DrawLineEx({cx - 8.5f, cy}, {cx + 8.5f, cy}, 2.6f, col);
            DrawLineEx({cx, cy - 8.5f}, {cx, cy + 8.5f}, 2.6f, col);
            break;
        }
        case 4: {
            DrawRectangleRoundedLinesEx({cx - 9.5f, cy - 7.5f, 19.0f, 15.0f}, 0.25f, 4, 1.6f, col);
            Vector2 wPts[5] = {
                {cx - 6.5f, cy + 1.0f},
                {cx - 3.0f, cy - 4.0f},
                {cx + 0.5f, cy + 4.0f},
                {cx + 3.8f, cy - 2.5f},
                {cx + 6.5f, cy + 0.5f}
            };
            for (int k = 0; k < 4; ++k) {
                DrawLineEx(wPts[k], wPts[k + 1], 1.8f, col);
            }
            break;
        }
        case 5: {
            Vector2 topArc[7], botArc[7];
            for (int k = 0; k <= 6; ++k) {
                float t = (float)k / 6.0f;
                float x = cx - 9.5f + t * 19.0f;
                float dy = std::sin(t * 3.14159265f) * 5.6f;
                topArc[k] = {x, cy - dy};
                botArc[k] = {x, cy + dy};
            }
            for (int k = 0; k < 6; ++k) {
                DrawLineEx(topArc[k], topArc[k + 1], 1.8f, col);
                DrawLineEx(botArc[k], botArc[k + 1], 1.8f, col);
            }
            DrawCircleV({cx, cy}, 2.7f, col);

            if (active) {
                DrawLineEx({cx - 8.5f, cy + 7.5f}, {cx + 8.5f, cy - 7.5f}, 2.3f, col);
            }
            break;
        }
        case 6: {
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
                           state.activeTool == ToolType::ADD_SPRING ||
                           state.activeTool == ToolType::ADD_PULLEY);

    const char* activeCreateName = "Создать объект [2]";
    if (state.activeTool == ToolType::ADD_BLOCK)   activeCreateName = "Создать: Брусок [2]";
    if (state.activeTool == ToolType::ADD_CIRCLE)  activeCreateName = "Создать: Шар [2]";
    if (state.activeTool == ToolType::ADD_SUPPORT) activeCreateName = "Создать: Опора [2]";
    if (state.activeTool == ToolType::ADD_ROPE)    activeCreateName = "Создать: Нить [2]";
    if (state.activeTool == ToolType::ADD_SPRING)  activeCreateName = "Создать: Пружина [2]";
    if (state.activeTool == ToolType::ADD_PULLEY)  activeCreateName = "Создать: Блок Атвуда [2]";

    const char* tooltips[7] = {
        state.paused ? "Запуск [Пробел]" : "Пауза [Пробел]",
        "Сброс [R]",
        "Курсор [1]",
        activeCreateName,
        "Осциллограф [3]",
        state.hideAllBodies ? "Показать тела [4 / H]" : "Скрыть тела (трассеры) [4 / H]",
        "Удалить [5]"
    };

    for (int i = 0; i < 7; ++i) {
        Rectangle btn = InputHandler::getDockButtonRect(i, sh);
        bool hovered = (state.hoveredDockBtn == i);
        bool active = false;

        if (i == 0) active = !state.paused;
        else if (i == 2) active = (state.activeTool == ToolType::SELECT_MOVE && !state.addMenuOpen);
        else if (i == 3) active = state.addMenuOpen || isCreationTool;
        else if (i == 4) active = state.showGraph;
        else if (i == 5) active = state.hideAllBodies;
        else if (i == 6) active = (state.activeTool == ToolType::DELETE_OBJ && !state.addMenuOpen);

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

    if (state.addMenuOpen) {
        Rectangle menu = InputHandler::getAddMenuRect(sh);

        DrawRectangleRounded({menu.x + 2.0f, menu.y + 4.0f, menu.width, menu.height}, 0.35f, 12, {10, 8, 8, 170});
        DrawRectangleRounded(menu, 0.35f, 12, {36, 30, 29, 242});
        DrawRectangleRoundedLinesEx(menu, 0.35f, 12, 1.6f, COL_GOLD);

        const char* names[6] = {"Брусок", "Шар", "Опора", "Нить", "Пружина", "Блок"};
        ToolType types[6] = {
            ToolType::ADD_BLOCK,
            ToolType::ADD_CIRCLE,
            ToolType::ADD_SUPPORT,
            ToolType::ADD_ROPE,
            ToolType::ADD_SPRING,
            ToolType::ADD_PULLEY
        };

        for (int i = 0; i < 6; ++i) {
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
    float topY = 10.0f;

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
            DrawTextEx(fontSmall, "* Shift + Колёсико — наклон опоры", {x, topY + 336}, 14, 1.0f, COL_TEXT_MUTED);

            drawButton({x, (float)sh - 48.0f, w, 32}, "Удалить опору", false, COL_WINE_BRIGHT);
            return;
        } else {
            DrawTextEx(fontBold, b.shape == ShapeType::CIRCLE ? TextFormat("Свойства шара #%d", b.id + 1) : TextFormat("Свойства бруска #%d", b.id + 1), {x, topY + 10}, 18, 1.0f, COL_GOLD);

            drawValueInputRow(x, topY + 40,  "Масса m", TextFormat("%.1f кг", b.mass),
                              state.editingField == EditField::BODY_MASS, state.inputBuffer);
            drawValueInputRow(x, topY + 90,  "Коэффициент трения \xCE\xBC", TextFormat("%.2f", b.friction),
                              state.editingField == EditField::BODY_FRICTION, state.inputBuffer);

            if (b.shape == ShapeType::CIRCLE) {
                drawValueInputRow(x, topY + 140, "Радиус шара R", TextFormat("%.2f м", b.radius),
                                  state.editingField == EditField::BODY_RADIUS, state.inputBuffer);
                drawValueInputRow(x, topY + 190, "Упругость отскока e", TextFormat("%.2f", b.restitution),
                                  state.editingField == EditField::BODY_RESTITUTION, state.inputBuffer);
            } else {
                drawValueInputRow(x, topY + 140, "Ширина W", TextFormat("%.2f м", b.width),
                                  state.editingField == EditField::BODY_WIDTH, state.inputBuffer);
                drawValueInputRow(x, topY + 190, "Высота H", TextFormat("%.2f м", b.height),
                                  state.editingField == EditField::BODY_HEIGHT, state.inputBuffer);
            }

            drawValueInputRow(x, topY + 240, "Внешняя сила тяги F", TextFormat("%.1f Н", b.appliedForceX),
                              state.editingField == EditField::BODY_FORCE_X, state.inputBuffer);

            drawButton({x, topY + 294, 130, 26}, b.canRotate ? "Вращение: Вкл" : "Вращение: Выкл", b.canRotate, COL_GOLD);
            drawButton({x + 138, topY + 294, 130, 26}, "Угол 0\xC2\xB0", false, COL_GOLD);

            drawButton({x, topY + 326, w, 26}, b.hidden ? "Тело скрыто (виден трассер)" : "Скрыть тело (оставить трассер)", b.hidden, COL_GOLD);

            DrawTextEx(fontMain, "Трассер тела:", {x, topY + 356}, 15, 1.0f, COL_TEXT_MUTED);
            float trBtnY = topY + 376.0f;
            drawButton({x,          trBtnY, 76.0f,  26.0f}, "Нет",        b.trailMode == 0, COL_GOLD);
            drawButton({x + 82.0f,  trBtnY, 104.0f, 26.0f}, "По времени", b.trailMode == 1, COL_GOLD);
            drawButton({x + 192.0f, trBtnY, 76.0f,  26.0f}, "Вечный",     b.trailMode == 2, COL_GOLD);

            float ty = topY + 410.0f;
            if (b.trailMode == 1) {
                drawValueInputRow(x, topY + 408.0f, "Удалять через", TextFormat("%.1f с", b.trailLifetime),
                                  state.editingField == EditField::BODY_TRAIL_LIFETIME, state.inputBuffer);
                ty = topY + 464.0f;
            }

            DrawLine((int)x, (int)ty, (int)(x + w), (int)ty, {64, 54, 50, 255});
            DrawTextEx(fontBold, "Телеметрия", {x, ty + 6}, 17, 1.0f, COL_GOLD);
            DrawTextEx(fontMain, TextFormat("Момент инерции I: %.2f кг*м\xC2\xB2", b.inertia), {x, ty + 26}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Угол / \xCF\x89: %.1f\xC2\xB0 (%.2f рад/с)", b.angle * 57.2958f, b.angVel), {x, ty + 44}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Скорость |v|: %.2f м/с", b.vel.length()), {x, ty + 62}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Сила тяжести mg: %.1f Н", b.mass * world.G), {x, ty + 80}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Реакция опоры N: %.1f Н", b.normalForce), {x, ty + 98}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Давление сверху P: %.1f Н", b.topLoadForce), {x, ty + 116}, 15, 1.0f, COL_TEXT_MUTED);
            DrawTextEx(fontMain, TextFormat("Сила трения Fтр: %.1f Н", b.frictionForce), {x, ty + 134}, 15, 1.0f, COL_TEXT_MUTED);

            drawButton({x, (float)sh - 48.0f, w, 32}, b.shape == ShapeType::CIRCLE ? "Удалить шар" : "Удалить брусок", false, COL_WINE_BRIGHT);
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

    drawButton({x,          topY + 4.0f, 130.0f, 28.0f}, "<- Меню", false, COL_GOLD);
    drawButton({x + 138.0f, topY + 4.0f, 130.0f, 28.0f},
               (state.screen == AppScreen::STUDY_LAB) ? "Каталог работ" : "Учебный центр", false, COL_GOLD);

    if (state.screen == AppScreen::STUDY_LAB) {
        int pIdx = std::clamp(world.activePreset - 1, 0, 7);
        DrawTextEx(fontBold, "Методическая карточка", {x, topY + 42.0f}, 18, 1.0f, COL_GOLD);

        Rectangle card = {x, topY + 66.0f, w, 214.0f};
        DrawRectangleRounded(card, 0.08f, 8, {24, 21, 20, 255});
        DrawRectangleRoundedLinesEx(card, 0.08f, 8, 1.2f, Fade(COL_GOLD, 0.45f));

        DrawTextEx(fontMain, LAB_META[pIdx].title, {x + 10.0f, topY + 74.0f}, 15, 1.0f, COL_GOLD);
        DrawTextEx(fontSmall, LAB_META[pIdx].topic, {x + 10.0f, topY + 96.0f}, 14, 1.0f, COL_TEXT_MUTED);
        DrawLine((int)(x + 10.0f), (int)(topY + 118.0f), (int)(x + w - 10.0f), (int)(topY + 118.0f), {58, 50, 46, 255});

        DrawTextEx(fontSmall, "Расчетная модель:", {x + 10.0f, topY + 124.0f}, 14, 1.0f, COL_TEXT_MUTED);
        DrawTextEx(fontSmall, LAB_META[pIdx].formula, {x + 10.0f, topY + 142.0f}, 14, 1.0f, COL_TEXT_CREAM);

        TelemetrySample cur = state.telemetryHistory.empty() ? TelemetrySample{} : state.telemetryHistory.back();
        DrawLine((int)(x + 10.0f), (int)(topY + 166.0f), (int)(x + w - 10.0f), (int)(topY + 166.0f), {58, 50, 46, 255});
        DrawTextEx(fontMain, TextFormat("Аналитика (Теория): %.2f", cur.theoryVal), {x + 10.0f, topY + 174.0f}, 15, 1.0f, COL_GOLD);
        DrawTextEx(fontMain, TextFormat("Расчет движка: %.2f", cur.simVal),         {x + 10.0f, topY + 196.0f}, 15, 1.0f, {96, 178, 224, 255});
        Color errCol = (cur.errorPct < 2.0f) ? Color{126, 186, 132, 255} : Color{214, 92, 92, 255};
        DrawTextEx(fontMain, TextFormat("Погрешность \xCE\xB4: %.2f%%", cur.errorPct), {x + 10.0f, topY + 218.0f}, 15, 1.0f, errCol);
        DrawTextEx(fontSmall, "* Кликните на тело для смены параметров", {x + 10.0f, topY + 252.0f}, 14, 1.0f, COL_TEXT_MUTED);

        DrawTextEx(fontMain, "Среда и решатель:", {x, topY + 290.0f}, 15, 1.0f, COL_TEXT_MUTED);
        drawButton({x,          topY + 312.0f, 130.0f, 28.0f}, "Вакуум", !world.airResistance, COL_GOLD);
        drawButton({x + 138.0f, topY + 312.0f, 130.0f, 28.0f}, "Воздух",  world.airResistance, COL_GOLD);

        DrawTextEx(fontMain, "Численный интегратор:", {x, topY + 346.0f}, 15, 1.0f, COL_TEXT_MUTED);
        drawButton({x,          topY + 366.0f, w,      26.0f}, "Симплектический Эйлер", world.integratorType == 0, COL_GOLD);
        drawButton({x,          topY + 396.0f, 130.0f, 26.0f}, "Явный Эйлер",           world.integratorType == 1, COL_GOLD);
        drawButton({x + 138.0f, topY + 396.0f, 130.0f, 26.0f}, "Метод Верле",           world.integratorType == 2, COL_GOLD);

        drawButton({x,          topY + 432.0f, w,      26.0f}, TextFormat("Центрировать камеру (%.0f%%)", state.camZoom * 100.0f), false, COL_GOLD);
        drawButton({x,          topY + 462.0f, 130.0f, 26.0f}, "Центр масс", state.showCenterOfMass, COL_GOLD);
        drawButton({x + 138.0f, topY + 462.0f, 130.0f, 26.0f}, "Профайлер",  state.showProfiler, COL_GOLD);

        drawButton({x,          topY + 498.0f, 130.0f, 28.0f}, "Осциллограф", state.showGraph, COL_GOLD);
        drawButton({x + 138.0f, topY + 498.0f, 130.0f, 28.0f}, "Сброс стенда", false, COL_GOLD);

        drawButton({x, topY + 534.0f, w, 28.0f}, "Открыть в песочнице", false, COL_WINE_BRIGHT);
        return;
    }

    DrawTextEx(fontBold, "Параметры среды", {x, topY + 42.0f}, 18, 1.0f, COL_GOLD);

    drawValueInputRow(x, topY + 68.0f, "Ускорение g", TextFormat("%.2f м/с\xC2\xB2", world.G),
                      state.editingField == EditField::WORLD_GRAVITY, state.inputBuffer);

    drawButton({x,          topY + 122.0f, 130.0f, 26.0f}, "g = 9.81", std::abs(world.G - 9.81f) < 0.02f, COL_GOLD);
    drawButton({x + 138.0f, topY + 122.0f, 130.0f, 26.0f}, "g = 10.0", std::abs(world.G - 10.0f) < 0.02f, COL_GOLD);

    drawButton({x,          topY + 154.0f, 130.0f, 26.0f}, "Вакуум", !world.airResistance, COL_GOLD);
    drawButton({x + 138.0f, topY + 154.0f, 130.0f, 26.0f}, "Воздух",  world.airResistance, COL_GOLD);

    drawValueInputRow(x, topY + 186.0f, "Точность расчета", TextFormat("%d шаг.", world.substeps),
                      state.editingField == EditField::WORLD_SUBSTEPS, state.inputBuffer);

    DrawTextEx(fontMain, "Численный интегратор:", {x, topY + 242.0f}, 15, 1.0f, COL_TEXT_MUTED);
    drawButton({x,          topY + 262.0f, w,      26.0f}, "Симплектический Эйлер", world.integratorType == 0, COL_GOLD);
    drawButton({x,          topY + 292.0f, 130.0f, 26.0f}, "Явный Эйлер",           world.integratorType == 1, COL_GOLD);
    drawButton({x + 138.0f, topY + 292.0f, 130.0f, 26.0f}, "Метод Верле",           world.integratorType == 2, COL_GOLD);

    DrawTextEx(fontMain, "Камера и диагностика:", {x, topY + 324.0f}, 15, 1.0f, COL_TEXT_MUTED);
    drawButton({x,          topY + 344.0f, w,      26.0f}, TextFormat("Центрировать камеру (%.0f%%)", state.camZoom * 100.0f), false, COL_GOLD);
    drawButton({x,          topY + 374.0f, 130.0f, 26.0f}, "Центр масс", state.showCenterOfMass, COL_GOLD);
    drawButton({x + 138.0f, topY + 374.0f, 130.0f, 26.0f}, "Профайлер",  state.showProfiler, COL_GOLD);

    DrawTextEx(fontMain, "Трассеры всех тел:", {x, topY + 406.0f}, 15, 1.0f, COL_TEXT_MUTED);
    float wTrBtnY = topY + 426.0f;
    drawButton({x,          wTrBtnY, 76.0f,  26.0f}, "Нет",        world.defaultTrailMode == 0, COL_GOLD);
    drawButton({x + 82.0f,  wTrBtnY, 104.0f, 26.0f}, "По времени", world.defaultTrailMode == 1, COL_GOLD);
    drawButton({x + 192.0f, wTrBtnY, 76.0f,  26.0f}, "Вечный",     world.defaultTrailMode == 2, COL_GOLD);

    float sepY = topY + 460.0f;
    if (world.defaultTrailMode == 1) {
        drawValueInputRow(x, topY + 456.0f, "Удалять через", TextFormat("%.1f с", world.defaultTrailLifetime),
                          state.editingField == EditField::WORLD_TRAIL_LIFETIME, state.inputBuffer);
        sepY = topY + 510.0f;
    }

    DrawLine((int)x, (int)sepY, (int)(x + w), (int)sepY, {64, 54, 50, 255});

    float botRow1 = sepY + 10.0f;
    drawButton({x,          botRow1, 130.0f, 28.0f}, "Демо-сцена",  false, COL_GOLD);
    drawButton({x + 138.0f, botRow1, 130.0f, 28.0f}, "Пустой лист", false, COL_WINE_BRIGHT);

    float botRow2 = botRow1 + 34.0f;
    drawButton({x,          botRow2, 130.0f, 28.0f}, "Сохранить", false, COL_GOLD);
    drawButton({x + 138.0f, botRow2, 130.0f, 28.0f}, "Загрузить", false, COL_GOLD);
}

void Renderer::drawHatchedSupport(const Body& b, int sw, int sh, bool highlighted, bool unlocked) const {
    float hx = b.width * 0.5f;
    float hy = b.height * 0.5f;
    float r = std::min(hy * 0.88f, hx * 0.45f);
    float innerX = std::max(0.0f, hx - r);
    float innerY = std::max(0.0f, hy - r);

    float bw = b.width * ppm;
    float bh = b.height * ppm;
    float rScr = r * ppm;
    float deg = -b.angle * 57.29578f;

    Vec2 localCorners[4] = {
        { innerX,  innerY},
        {-innerX,  innerY},
        {-innerX, -innerY},
        { innerX, -innerY}
    };

    auto drawRoundedShape = [&](Vector2 offsetPx, Color col) {
        Vector2 cScr = worldToScreen(b.pos, sw, sh);
        cScr.x += offsetPx.x;
        cScr.y += offsetPx.y;

        if (innerX > 1e-4f) {
            DrawRectanglePro({cScr.x, cScr.y, innerX * 2.0f * ppm, bh}, {innerX * ppm, bh * 0.5f}, deg, col);
        }
        if (innerY > 1e-4f) {
            DrawRectanglePro({cScr.x, cScr.y, bw, innerY * 2.0f * ppm}, {bw * 0.5f, innerY * ppm}, deg, col);
        }
        for (int i = 0; i < 4; ++i) {
            Vector2 p = worldToScreen(b.localToWorld(localCorners[i]), sw, sh);
            DrawCircleV({p.x + offsetPx.x, p.y + offsetPx.y}, rScr, col);
        }
    };

    drawRoundedShape({2.0f, 4.0f}, {14, 12, 12, 255});
    Color fillCol = unlocked ? Color{52, 28, 34, 255} : Color{34, 29, 28, 255};
    drawRoundedShape({0.0f, 0.0f}, fillCol);

    auto getMaxYAtX = [&](float x) {
        float ax = std::abs(x);
        if (ax >= hx) return 0.0f;
        if (ax <= innerX) return hy;
        float dx = ax - innerX;
        return innerY + std::sqrt(std::max(0.0f, r * r - dx * dx));
    };

    Color hatchCol = highlighted ? COL_GOLD : Fade(COL_GOLD_DIM, 0.78f);
    float step = 0.18f;
    float slant = hy * 0.65f;
    for (float x = -hx - slant + 0.06f; x <= hx + slant - 0.06f; x += step) {
        float x1 = std::clamp(x - slant, -hx + 0.015f, hx - 0.015f);
        float x2 = std::clamp(x + slant, -hx + 0.015f, hx - 0.015f);
        if (x2 - x1 < 0.02f) continue;

        float yLim1 = std::max(0.0f, getMaxYAtX(x1) - 0.018f);
        float yLim2 = std::max(0.0f, getMaxYAtX(x2) - 0.018f);
        if (yLim1 <= 0.005f && yLim2 <= 0.005f) continue;

        Vec2 pBot = b.localToWorld({x1, -yLim1});
        Vec2 pTop = b.localToWorld({x2,  yLim2});
        DrawLineEx(worldToScreen(pBot, sw, sh), worldToScreen(pTop, sw, sh), 1.4f, hatchCol);
    }

    const int ARC_PTS = 8;
    std::vector<Vector2> perim;
    perim.reserve(ARC_PTS * 4);
    float startAngles[4] = {0.0f, 1.5707963f, 3.1415926f, 4.712389f};

    for (int c = 0; c < 4; ++c) {
        for (int s = 0; s < ARC_PTS; ++s) {
            float a = startAngles[c] + (float)s / (float)(ARC_PTS - 1) * 1.5707963f;
            Vec2 lp = {
                localCorners[c].x + std::cos(a) * r,
                localCorners[c].y + std::sin(a) * r
            };
            perim.push_back(worldToScreen(b.localToWorld(lp), sw, sh));
        }
    }

    Color borderCol = highlighted ? COL_GOLD : (unlocked ? COL_WINE_BRIGHT : COL_TEXT_CREAM);
    float thick = highlighted ? 2.4f : 1.8f;
    for (size_t i = 0; i < perim.size(); ++i) {
        Vector2 p1 = perim[i];
        Vector2 p2 = perim[(i + 1) % perim.size()];
        DrawLineEx(p1, p2, thick, borderCol);
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
    camPos = state.camPos;
    ppm = basePpm * state.camZoom;

    BeginDrawing();
    ClearBackground(COL_BG_DARK);

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    if (state.screen == AppScreen::MAIN_MENU) {
        drawMainMenu(sw, sh);
        EndDrawing();
        return;
    }
    if (state.screen == AppScreen::STUDY_HUB) {
        drawStudyHub(world, sw, sh);
        EndDrawing();
        return;
    }
    if (state.screen == AppScreen::SETTINGS) {
        drawSettingsScreen(world, state, sw, sh);
        EndDrawing();
        return;
    }
    if (state.screen == AppScreen::ABOUT) {
        drawAboutScreen(sw, sh);
        EndDrawing();
        return;
    }

    int canvasW = sw - InputHandler::INSPECTOR_WIDTH;

    BeginScissorMode(0, 0, canvasW, sh);

    drawInfiniteGridAndFloor(world, sw, sh);
    drawTrails(world, state, sw, sh);
    drawPulleys(world, state, sw, sh);

    if (!state.hideAllBodies) {
        for (const auto& r : world.ropes) {
            const Body& a = world.bodies[r.idA];
            Vector2 sA = worldToScreen(a.localToWorld(r.offsetA), sw, sh);
            Vector2 sB = worldToScreen(world.getRopeEndWorldPos(r), sw, sh);

            bool isHighlighted = (r.id == state.hoveredRopeId) || (r.id == state.selectedRopeId);
            Color ropeCol = isHighlighted ? COL_TEXT_CREAM : ((r.tension > 0.5f) ? COL_GOLD : COL_GOLD_DIM);

            if (r.isSpring && !r.isRigid) {
                if (!isHighlighted && r.signedExtension() < -0.02f) {
                    ropeCol = {222, 146, 96, 255};
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
    }

    if ((state.activeTool == ToolType::ADD_ROPE || state.activeTool == ToolType::ADD_SPRING || state.activeTool == ToolType::ADD_PULLEY) &&
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
        DrawTextEx(fontMain, (state.activeTool == ToolType::ADD_PULLEY) ? "Выберите 2-й груз..." : TextFormat("L0 = %.2f м", dragLen),
                   {sB.x + 12, sB.y - 10}, 15, 1.0f, COL_GOLD);
    }

    for (const auto& b : world.bodies) {
        if (!b.isStatic && (state.hideAllBodies || b.hidden)) continue;
        if (b.isStatic && state.hideAllBodies) continue;

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

        if ((state.activeTool == ToolType::ADD_ROPE || state.activeTool == ToolType::ADD_SPRING || state.activeTool == ToolType::ADD_PULLEY) &&
            b.id == state.hoveredBodyId) {
            Vec2 snap = world.getSnapOffset(b.id, state.mouseWorldPos);
            Vector2 snapScr = worldToScreen(b.localToWorld(snap), sw, sh);
            DrawCircleV(snapScr, 5.5f, COL_GOLD);
        }
    }

    drawCenterOfMass(world, state, sw, sh);

    Vector2 m = GetMousePosition();
    Rectangle dockRect   = InputHandler::getDockRect(sh);
    Rectangle menuRect   = InputHandler::getAddMenuRect(sh);
    Rectangle chronoRect = InputHandler::getChronoBarRect(sw);
    Rectangle graphRect  = InputHandler::getGraphRect(state, sw, sh);

    bool overUI = (m.x >= canvasW) ||
                  (m.x >= dockRect.x && m.x <= dockRect.x + dockRect.width && m.y >= dockRect.y && m.y <= dockRect.y + dockRect.height) ||
                  (m.x >= chronoRect.x && m.x <= chronoRect.x + chronoRect.width && m.y >= chronoRect.y && m.y <= chronoRect.y + chronoRect.height) ||
                  (state.showGraph && m.x >= graphRect.x && m.x <= graphRect.x + graphRect.width && m.y >= graphRect.y && m.y <= graphRect.y + graphRect.height) ||
                  (state.addMenuOpen && m.x >= menuRect.x && m.x <= menuRect.x + menuRect.width && m.y >= menuRect.y && m.y <= menuRect.y + menuRect.height);

    bool isCreationTool = (state.activeTool == ToolType::ADD_BLOCK ||
                           state.activeTool == ToolType::ADD_CIRCLE ||
                           state.activeTool == ToolType::ADD_SUPPORT ||
                           state.activeTool == ToolType::ADD_ROPE ||
                           state.activeTool == ToolType::ADD_SPRING ||
                           state.activeTool == ToolType::ADD_PULLEY);

    if (state.creatingSupportBodyId >= 0 && state.creatingSupportBodyId < (int)world.bodies.size()) {
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
            float h = 0.24f * ppm;
            Rectangle sup = {m.x - w * 0.5f, m.y - h * 0.5f, w, h};
            DrawRectangleRounded(sup, 0.85f, 8, Fade(COL_PANEL_BG, 0.65f));
            DrawRectangleRoundedLinesEx(sup, 0.85f, 8, 1.5f, Fade(COL_GOLD, 0.75f));
        }

        int iconIdx = 0;
        const char* label = "+ Брусок";
        if (state.activeTool == ToolType::ADD_CIRCLE)  { iconIdx = 1; label = "+ Шар"; }
        if (state.activeTool == ToolType::ADD_SUPPORT) { iconIdx = 2; label = "+ Опора (тяни ЛКМ)"; }
        if (state.activeTool == ToolType::ADD_ROPE)    { iconIdx = 3; label = "+ Нить"; }
        if (state.activeTool == ToolType::ADD_SPRING)  { iconIdx = 4; label = "+ Пружина"; }
        if (state.activeTool == ToolType::ADD_PULLEY)  { iconIdx = 5; label = "+ Блок (2 груза)"; }

        Vector2 ts = MeasureTextEx(fontSmall, label, 14, 1.0f);
        Rectangle tag = {m.x + 14.0f, m.y + 14.0f, ts.x + 34.0f, 24.0f};
        DrawRectangleRounded(tag, 0.5f, 6, {28, 24, 23, 225});
        DrawRectangleRoundedLinesEx(tag, 0.5f, 6, 1.0f, Fade(COL_GOLD, 0.75f));
        drawCreationIcon(iconIdx, {tag.x + 14.0f, tag.y + 12.0f}, COL_GOLD, true);
        DrawTextEx(fontSmall, label, {tag.x + 26.0f, tag.y + (tag.height - ts.y) * 0.5f}, 14, 1.0f, COL_TEXT_CREAM);
    }

    const float FORCE_SCALE = 0.9f;
    if (state.forceViewMode != 2 && !state.hideAllBodies) {
        for (const auto& b : world.bodies) {
            if (b.isStatic || b.hidden) continue;

            Vector2 scr = worldToScreen(b.pos, sw, sh);
            float bw = (b.shape == ShapeType::CIRCLE ? (b.radius * 2.0f) : b.width) * ppm;
            float bh = (b.shape == ShapeType::CIRCLE ? (b.radius * 2.0f) : b.height) * ppm;

            bool hasAppliedForce = (std::abs(b.appliedForceX) > 0.1f);
            bool isHighlighted = (b.id == state.hoveredBodyId) || (b.id == state.draggedBodyId);

            if (hasAppliedForce) {
                float sign = (b.appliedForceX > 0.0f) ? 1.0f : -1.0f;
                Vector2 sidePt = {scr.x + sign * bw * 0.5f, scr.y};
                Vector2 endPt  = {sidePt.x + b.appliedForceX * FORCE_SCALE, sidePt.y};
                drawVectorArrow(sidePt, endPt, {96, 178, 224, 255},
                                TextFormat("F = %.1f Н", std::abs(b.appliedForceX)),
                                {sign > 0 ? 8.0f : -80.0f, -18.0f});
            }

            Vec2 nVec = b.contactNormal;
            Vec2 tVec = {nVec.y, -nVec.x};

            if ((state.forceViewMode == 1 || isHighlighted || hasAppliedForce) && b.frictionForce > 0.2f) {
                float vTang = Vec2::dot(b.vel, tVec);
                float dirSign = (std::abs(vTang) > 0.05f) ? ((vTang > 0.0f) ? -1.0f : 1.0f)
                              : (hasAppliedForce ? ((b.appliedForceX > 0.0f) ? -1.0f : 1.0f) : -1.0f);
                Vec2 fDir = tVec * dirSign;
                Vector2 frStart = {scr.x - nVec.x * (bh * 0.5f - 2.0f), scr.y + nVec.y * (bh * 0.5f - 2.0f)};
                Vector2 frEnd   = {frStart.x + fDir.x * b.frictionForce * FORCE_SCALE, frStart.y - fDir.y * b.frictionForce * FORCE_SCALE};
                drawVectorArrow(frStart, frEnd, {188, 122, 214, 255},
                                TextFormat("Fтр = %.1f Н", b.frictionForce),
                                {fDir.x >= 0 ? 8.0f : -90.0f, 6.0f});
            }

            if (state.forceViewMode == 0 && !isHighlighted) continue;

            float fg = b.mass * world.G;
            drawVectorArrow(scr, {scr.x, scr.y + fg * FORCE_SCALE}, {214, 92, 92, 255},
                            TextFormat("mg = %.1f Н", fg), {10.0f, -8.0f});

            if (b.normalForce > 0.2f) {
                Vector2 bottomPt = {scr.x - nVec.x * (bh * 0.5f) - 4.0f, scr.y + nVec.y * (bh * 0.5f)};
                Vector2 endN = {bottomPt.x + nVec.x * b.normalForce * FORCE_SCALE, bottomPt.y - nVec.y * b.normalForce * FORCE_SCALE};
                drawVectorArrow(bottomPt, endN, {126, 186, 132, 255}, TextFormat("N = %.1f Н", b.normalForce), {-95.0f, -8.0f});
            }

            if (b.topLoadForce > 0.2f) {
                Vector2 topPt = {scr.x + 6.0f, scr.y - bh * 0.5f};
                drawVectorArrow(topPt, {topPt.x, topPt.y + b.topLoadForce * FORCE_SCALE},
                                {218, 144, 88, 255}, TextFormat("P = %.1f Н", b.topLoadForce), {10.0f, -8.0f});
            }
        }
    }

    if (!state.hideAllBodies) {
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
    }

    float hudY = 18.0f;
    const char* forceModeStr = (state.forceViewMode == 0) ? "[V] - силы (при наведении)"
                             : (state.forceViewMode == 1) ? "[V] - силы (все)"
                                                          : "[V] - силы (скрыты)";

    DrawTextEx(fontMain, state.paused ? "[Пробел] - пауза" : "[Пробел] - симуляция", {20, hudY},       15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "[ПКМ / Колесо] - камера",                                 {20, hudY + 22},  15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, forceModeStr,                                              {20, hudY + 44},  15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "[R] - сброс сцены",                                       {20, hudY + 66},  15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, state.hideAllBodies ? "[H] - показать тела" : "[H] - скрыть тела", {20, hudY + 88}, 15, 1.0f, COL_TEXT_MUTED);
    DrawTextEx(fontMain, "[Esc] - главное меню",                                    {20, hudY + 110}, 15, 1.0f, COL_TEXT_MUTED);

    drawProfilerOverlay(world, state);
    drawChronoBar(world, state, sw);
    drawOscilloscope(world, state, sw, sh);

    EndScissorMode();

    drawLiquidDock(sh, state);
    drawInspector(world, state, sw, sh);

    EndDrawing();
}