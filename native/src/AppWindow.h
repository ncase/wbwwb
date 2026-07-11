#pragma once

#include "Settings.h"
#include "render/Renderer.h"

#include <string>
#include <vector>

namespace wb {

class AppWindow {
public:
    virtual ~AppWindow() = default;

    virtual void initialize(const GameSettings& settings, const char* title) = 0;
    virtual void shutdown() = 0;
    virtual void applyIcons(const std::string& iconPath) = 0;

    virtual void setTargetFps(int fps) = 0;
    virtual float frameTime() const = 0;
    virtual void setVsync(bool enabled) = 0;
    virtual int clientWidth() const = 0;
    virtual int clientHeight() const = 0;

    virtual int monitorCount() const = 0;
    virtual int currentMonitor() const = 0;
    virtual std::string monitorName(int index) const = 0;
    virtual int monitorWidth(int index) const = 0;
    virtual int monitorHeight(int index) const = 0;
    virtual render::Vec2 monitorPosition(int index) const = 0;
    virtual std::vector<WindowSizeOption> monitorWindowSizes() const = 0;

    virtual bool isFullscreen() const = 0;
    virtual void setUndecorated(bool enabled) = 0;
    virtual void setWindowPosition(int x, int y) = 0;
    virtual void setWindowSize(int width, int height) = 0;
    virtual void setFullscreen(bool enabled) = 0;
};

}  // namespace wb
