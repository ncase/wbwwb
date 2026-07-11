#pragma once

#include "AppWindow.h"

#include <chrono>
#include <cstdint>

namespace wbwwb::vulkan {
class VulkanBackend;
}

namespace wb {

class VulkanAppWindow final : public AppWindow {
public:
    VulkanAppWindow(void* instance, void* window, wbwwb::vulkan::VulkanBackend& backend);

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
    void syncBackendToClient() const;

    void* instance_ = nullptr;
    void* window_ = nullptr;
    wbwwb::vulkan::VulkanBackend& backend_;
    mutable std::chrono::steady_clock::time_point lastFrame_ = std::chrono::steady_clock::now();
    int targetFps_ = 60;
    bool initialized_ = false;
    bool fullscreen_ = false;
    bool undecorated_ = false;
    std::intptr_t restoredStyle_ = 0;
    int restoredX_ = 0;
    int restoredY_ = 0;
    int restoredWidth_ = 960;
    int restoredHeight_ = 540;
};

}  // namespace wb
