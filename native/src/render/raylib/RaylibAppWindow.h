#pragma once

#include "AppWindow.h"

namespace wb {

class RaylibAppWindow final : public AppWindow {
public:
    RaylibAppWindow() = default;
    ~RaylibAppWindow() override;

    void initialize(const GameSettings& settings, const char* title) override;
    void shutdown() override;
    void applyIcons(const std::string& iconPath) override;

    void setTargetFps(int fps) override;
    float frameTime() const override;
    void setVsync(bool enabled) override;
    int clientWidth() const override;
    int clientHeight() const override;

    int monitorCount() const override;
    int currentMonitor() const override;
    std::string monitorName(int index) const override;
    int monitorWidth(int index) const override;
    int monitorHeight(int index) const override;
    render::Vec2 monitorPosition(int index) const override;
    std::vector<WindowSizeOption> monitorWindowSizes() const override;

    bool isFullscreen() const override;
    void setUndecorated(bool enabled) override;
    void setWindowPosition(int x, int y) override;
    void setWindowSize(int width, int height) override;
    void setFullscreen(bool enabled) override;

private:
    bool initialized_ = false;
};

}  // namespace wb
