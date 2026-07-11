#include "compat/RaylibCompat.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

namespace wb {
namespace {

RaylibCompatContext g_context{};
bool g_hasContext = false;

Color withAlpha(Color color, float alpha) {
    color.a = static_cast<std::uint8_t>(
        std::clamp(alpha, 0.0f, 1.0f) * static_cast<float>(color.a)
    );
    return color;
}

render::Renderer* rendererOrNull() {
    return g_hasContext ? g_context.renderer : nullptr;
}

const char* levelName(int level) {
    switch (level) {
        case LOG_TRACE: return "trace";
        case LOG_DEBUG: return "debug";
        case LOG_INFO: return "info";
        case LOG_WARNING: return "warning";
        case LOG_ERROR: return "error";
        default: return "log";
    }
}

float roundedRadius(Rectangle rect, float roundness) {
    const float diameter = std::min(std::abs(rect.width), std::abs(rect.height)) * std::clamp(roundness, 0.0f, 1.0f);
    return std::max(0.0f, diameter * 0.5f);
}

void drawArcLines(render::Renderer& renderer, Vector2 center, float radius, float startRadians, float endRadians, int segments, Color color) {
    const int count = std::max(2, segments);
    Vector2 previous{
        center.x + std::cos(startRadians) * radius,
        center.y + std::sin(startRadians) * radius
    };
    for (int i = 1; i <= count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(count);
        const float angle = startRadians + (endRadians - startRadians) * t;
        const Vector2 current{
            center.x + std::cos(angle) * radius,
            center.y + std::sin(angle) * radius
        };
        renderer.DrawLine(compat::toRender(previous), compat::toRender(current), 1.0f, compat::toRender(color));
        previous = current;
    }
}

}  // namespace

void setRaylibCompatContext(const RaylibCompatContext& context) {
    g_context = context;
    g_hasContext = true;
}

void clearRaylibCompatContext() {
    g_context = {};
    g_hasContext = false;
}

RaylibCompatContext* raylibCompatContext() {
    return g_hasContext ? &g_context : nullptr;
}

const RaylibCompatContext* raylibCompatContextConst() {
    return g_hasContext ? &g_context : nullptr;
}

namespace compat {

render::Vec2 toRender(Vector2 value) {
    return {value.x, value.y};
}

render::Rect toRender(Rectangle value) {
    return {value.x, value.y, value.width, value.height};
}

render::Color toRender(Color value) {
    return {value.r, value.g, value.b, value.a};
}

}  // namespace compat

bool CheckCollisionPointRec(Vector2 point, Rectangle rect) {
    return point.x >= rect.x &&
           point.x <= rect.x + rect.width &&
           point.y >= rect.y &&
           point.y <= rect.y + rect.height;
}

void TraceLog(int level, const char* format, ...) {
    std::fprintf(stderr, "[wbwwb:%s] ", levelName(level));
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fprintf(stderr, "\n");
}

void OpenURL(const char* url) {
    if (!url || url[0] == '\0') {
        return;
    }

#if defined(_WIN32)
    const INT_PTR result = reinterpret_cast<INT_PTR>(
        ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL)
    );
    if (result <= 32) {
        TraceLog(LOG_WARNING, "Could not open URL: %s", url);
    }
#else
    std::string command = "xdg-open \"";
    for (const char* cursor = url; *cursor != '\0'; ++cursor) {
        if (*cursor == '"' || *cursor == '\\' || *cursor == '$' || *cursor == '`') {
            command.push_back('\\');
        }
        command.push_back(*cursor);
    }
    command += "\"";
    if (std::system(command.c_str()) != 0) {
        TraceLog(LOG_WARNING, "Could not open URL: %s", url);
    }
#endif
}

void DrawLineEx(Vector2 start, Vector2 end, float thick, Color color) {
    if (render::Renderer* renderer = rendererOrNull()) {
        renderer->DrawLine(compat::toRender(start), compat::toRender(end), thick, compat::toRender(color));
    }
}

void DrawCircleV(Vector2 center, float radius, Color color) {
    if (render::Renderer* renderer = rendererOrNull()) {
        renderer->DrawCircle(compat::toRender(center), radius, compat::toRender(color));
    }
}

void DrawCircleLines(int centerX, int centerY, float radius, Color color) {
    if (render::Renderer* renderer = rendererOrNull()) {
        constexpr int kSegments = 48;
        Vector2 previous{static_cast<float>(centerX) + radius, static_cast<float>(centerY)};
        for (int i = 1; i <= kSegments; ++i) {
            const float angle = (static_cast<float>(i) / static_cast<float>(kSegments)) * PI * 2.0f;
            const Vector2 current{
                static_cast<float>(centerX) + std::cos(angle) * radius,
                static_cast<float>(centerY) + std::sin(angle) * radius
            };
            renderer->DrawLine(compat::toRender(previous), compat::toRender(current), 1.0f, compat::toRender(color));
            previous = current;
        }
    }
}

void DrawRectangleRounded(Rectangle rect, float roundness, int, Color color) {
    render::Renderer* renderer = rendererOrNull();
    if (!renderer) {
        return;
    }

    const float radius = roundedRadius(rect, roundness);
    if (radius <= 0.0f) {
        renderer->DrawRectangle(compat::toRender(rect), compat::toRender(color));
        return;
    }

    renderer->DrawRectangle({rect.x + radius, rect.y, rect.width - radius * 2.0f, rect.height}, compat::toRender(color));
    renderer->DrawRectangle({rect.x, rect.y + radius, rect.width, rect.height - radius * 2.0f}, compat::toRender(color));
    renderer->DrawCircle({rect.x + radius, rect.y + radius}, radius, compat::toRender(color));
    renderer->DrawCircle({rect.x + rect.width - radius, rect.y + radius}, radius, compat::toRender(color));
    renderer->DrawCircle({rect.x + radius, rect.y + rect.height - radius}, radius, compat::toRender(color));
    renderer->DrawCircle({rect.x + rect.width - radius, rect.y + rect.height - radius}, radius, compat::toRender(color));
}

void DrawRectangleRoundedLines(Rectangle rect, float roundness, int segments, Color color) {
    render::Renderer* renderer = rendererOrNull();
    if (!renderer) {
        return;
    }

    const float radius = roundedRadius(rect, roundness);
    if (radius <= 0.0f) {
        renderer->DrawRectangleOutline(compat::toRender(rect), 1.0f, compat::toRender(color));
        return;
    }

    const int arcSegments = std::max(2, segments / 4);
    const float left = rect.x;
    const float right = rect.x + rect.width;
    const float top = rect.y;
    const float bottom = rect.y + rect.height;

    renderer->DrawLine({left + radius, top}, {right - radius, top}, 1.0f, compat::toRender(color));
    renderer->DrawLine({right, top + radius}, {right, bottom - radius}, 1.0f, compat::toRender(color));
    renderer->DrawLine({right - radius, bottom}, {left + radius, bottom}, 1.0f, compat::toRender(color));
    renderer->DrawLine({left, bottom - radius}, {left, top + radius}, 1.0f, compat::toRender(color));

    drawArcLines(*renderer, {right - radius, top + radius}, radius, -PI / 2.0f, 0.0f, arcSegments, color);
    drawArcLines(*renderer, {right - radius, bottom - radius}, radius, 0.0f, PI / 2.0f, arcSegments, color);
    drawArcLines(*renderer, {left + radius, bottom - radius}, radius, PI / 2.0f, PI, arcSegments, color);
    drawArcLines(*renderer, {left + radius, top + radius}, radius, PI, PI * 1.5f, arcSegments, color);
}

}  // namespace wb
