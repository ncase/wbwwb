#pragma once

#include "render/Renderer.h"

#include <cstdint>

#ifdef PI
#undef PI
#endif
#ifdef DEG2RAD
#undef DEG2RAD
#endif
#ifdef RAD2DEG
#undef RAD2DEG
#endif
#ifdef WHITE
#undef WHITE
#endif
#ifdef BLACK
#undef BLACK
#endif
#ifdef RED
#undef RED
#endif
#ifdef GREEN
#undef GREEN
#endif
#ifdef BLUE
#undef BLUE
#endif
#ifdef BLANK
#undef BLANK
#endif

namespace wb {

class AssetManager;
class AudioManager;
class CaptureService;
class FramePipeline;
class InputProvider;
class Localization;
class GameSettings;

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Rectangle {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

inline constexpr float PI = 3.14159265358979323846f;
inline constexpr float DEG2RAD = PI / 180.0f;
inline constexpr float RAD2DEG = 180.0f / PI;

inline constexpr Color WHITE{255, 255, 255, 255};
inline constexpr Color BLACK{0, 0, 0, 255};
inline constexpr Color RED{230, 41, 55, 255};
inline constexpr Color GREEN{0, 228, 48, 255};
inline constexpr Color BLUE{0, 121, 241, 255};
inline constexpr Color BLANK{0, 0, 0, 0};

inline constexpr int LOG_TRACE = 1;
inline constexpr int LOG_DEBUG = 2;
inline constexpr int LOG_INFO = 3;
inline constexpr int LOG_WARNING = 4;
inline constexpr int LOG_ERROR = 5;

struct RaylibCompatContext {
    render::Renderer* renderer = nullptr;
    InputProvider* input = nullptr;
    AudioManager* audio = nullptr;
    CaptureService* capture = nullptr;
    AssetManager* assets = nullptr;
    FramePipeline* framePipeline = nullptr;
    Localization* localization = nullptr;
    GameSettings* settings = nullptr;
};

void setRaylibCompatContext(const RaylibCompatContext& context);
void clearRaylibCompatContext();
RaylibCompatContext* raylibCompatContext();
const RaylibCompatContext* raylibCompatContextConst();

namespace compat {

render::Vec2 toRender(Vector2 value);
render::Rect toRender(Rectangle value);
render::Color toRender(Color value);

}  // namespace compat

bool CheckCollisionPointRec(Vector2 point, Rectangle rect);
void TraceLog(int level, const char* format, ...);
void OpenURL(const char* url);

void DrawLineEx(Vector2 start, Vector2 end, float thick, Color color);
void DrawCircleV(Vector2 center, float radius, Color color);
void DrawCircleLines(int centerX, int centerY, float radius, Color color);
void DrawRectangleRounded(Rectangle rect, float roundness, int segments, Color color);
void DrawRectangleRoundedLines(Rectangle rect, float roundness, int segments, Color color);

}  // namespace wb
