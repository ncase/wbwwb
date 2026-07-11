#include "render/raylib/RaylibAppWindow.h"

#include "Constants.h"
#include "WindowIcon.h"

#include "raylib.h"

#include <algorithm>
#include <array>

namespace wb {
namespace {

void addMonitorWindowSize(std::vector<WindowSizeOption>& options, int width, int height) {
    if (width >= 640 && height >= 360 && width <= 7680 && height <= 4320) {
        options.push_back({width, height, {}});
    }
}

void applyRaylibWindowIcons(const std::string& iconPath) {
    Image source = LoadImage(iconPath.c_str());
    if (!source.data) {
        return;
    }
    ImageFormat(&source, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    constexpr std::array<int, 4> kIconSizes{16, 32, 48, 256};
    std::array<Image, kIconSizes.size()> icons{};
    for (std::size_t i = 0; i < kIconSizes.size(); ++i) {
        icons[i] = ImageCopy(source);
        ImageResize(&icons[i], kIconSizes[i], kIconSizes[i]);
    }

    SetWindowIcons(icons.data(), static_cast<int>(icons.size()));

    for (Image& icon : icons) {
        UnloadImage(icon);
    }
    UnloadImage(source);
}

}  // namespace

RaylibAppWindow::~RaylibAppWindow() {
    shutdown();
}

void RaylibAppWindow::initialize(const GameSettings& settings, const char* title) {
    if (initialized_) {
        return;
    }

    unsigned int flags = FLAG_WINDOW_RESIZABLE;
    if (settings.vsync) {
        flags |= FLAG_VSYNC_HINT;
    }
    SetConfigFlags(flags);

    const WindowSizeOption& window = settings.currentWindowSize();
    InitWindow(window.width, window.height, title);
    initialized_ = true;
}

void RaylibAppWindow::shutdown() {
    if (!initialized_) {
        return;
    }
    CloseWindow();
    initialized_ = false;
}

void RaylibAppWindow::applyIcons(const std::string& iconPath) {
    applyRaylibWindowIcons(iconPath);
    applyEmbeddedWindowIcon(GetWindowHandle());
}

void RaylibAppWindow::setTargetFps(int fps) {
    SetTargetFPS(fps);
}

float RaylibAppWindow::frameTime() const {
    return GetFrameTime();
}

void RaylibAppWindow::setVsync(bool enabled) {
    if (enabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
}

int RaylibAppWindow::clientWidth() const {
    return GetScreenWidth();
}

int RaylibAppWindow::clientHeight() const {
    return GetScreenHeight();
}

int RaylibAppWindow::monitorCount() const {
    return std::max(1, GetMonitorCount());
}

int RaylibAppWindow::currentMonitor() const {
    return GetCurrentMonitor();
}

std::string RaylibAppWindow::monitorName(int index) const {
    const char* name = GetMonitorName(index);
    return name != nullptr ? std::string(name) : std::string();
}

int RaylibAppWindow::monitorWidth(int index) const {
    return GetMonitorWidth(index);
}

int RaylibAppWindow::monitorHeight(int index) const {
    return GetMonitorHeight(index);
}

render::Vec2 RaylibAppWindow::monitorPosition(int index) const {
    const ::Vector2 position = GetMonitorPosition(index);
    return {position.x, position.y};
}

std::vector<WindowSizeOption> RaylibAppWindow::monitorWindowSizes() const {
    std::vector<WindowSizeOption> options;
    const int count = monitorCount();
    for (int i = 0; i < count; ++i) {
        addMonitorWindowSize(options, GetMonitorWidth(i), GetMonitorHeight(i));
    }
    return options;
}

bool RaylibAppWindow::isFullscreen() const {
    return IsWindowFullscreen();
}

void RaylibAppWindow::setUndecorated(bool enabled) {
    if (enabled) {
        SetWindowState(FLAG_WINDOW_UNDECORATED);
    } else {
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
    }
}

void RaylibAppWindow::setWindowPosition(int x, int y) {
    SetWindowPosition(x, y);
}

void RaylibAppWindow::setWindowSize(int width, int height) {
    SetWindowSize(width, height);
}

void RaylibAppWindow::setFullscreen(bool enabled) {
    if (IsWindowFullscreen() != enabled) {
        ToggleFullscreen();
    }
}

}  // namespace wb
