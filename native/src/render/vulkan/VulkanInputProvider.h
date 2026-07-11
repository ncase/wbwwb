#pragma once

#include "InputProvider.h"

#include <array>
#include <cstdint>
#include <vector>

namespace wb {

struct VulkanInputTestScript {
    std::uint64_t maxFrames = 0;
    std::vector<std::uint64_t> enterFrames;
    std::vector<std::uint64_t> downFrames;
    std::vector<std::uint64_t> rightFrames;
    std::vector<std::uint64_t> escapeFrames;
    std::vector<std::uint64_t> leftClickFrames;
    bool overrideLogicalMouse = false;
    render::Vec2 logicalMouse{278.0f, 250.0f};
};

class VulkanInputProvider final : public InputProvider {
public:
    VulkanInputProvider(void* window, bool* shouldClose, float* wheelDelta, VulkanInputTestScript script = {});

    void updateLogicalMapping(render::Rect viewport, render::Vec2 logicalSize) override;
    render::Vec2 logicalMousePosition() const override;
    render::Vec2 windowMousePosition() const override;
    bool mouseInLogicalArea() const override;
    bool mouseButtonDown(MouseButton button) const override;
    bool mouseButtonPressed(MouseButton button) const override;
    bool mouseButtonReleased(MouseButton button) const override;
    bool keyDown(InputKey key) const override;
    bool keyPressed(InputKey key) const override;
    bool keyReleased(InputKey key) const override;
    int typedCharacter() const override;
    float scrollWheelMove() const override;
    bool windowCloseRequested() const override;

    std::uint64_t frameCount() const { return frameCount_; }

private:
    void pollFrame() const;

    void* window_ = nullptr;
    bool* shouldClose_ = nullptr;
    float* wheelDelta_ = nullptr;
    VulkanInputTestScript script_;
    render::Rect viewport_{};
    render::Vec2 logicalSize_{960.0f, 540.0f};
    mutable render::Vec2 logicalMouse_{};
    mutable render::Vec2 windowMouse_{};
    mutable bool mouseInLogicalArea_ = false;
    mutable std::array<bool, 3> mouseDown_{};
    mutable std::array<bool, 3> mousePressed_{};
    mutable std::array<bool, 3> mouseReleased_{};
    mutable std::array<bool, 7> keyDown_{};
    mutable std::array<bool, 7> keyPressed_{};
    mutable std::array<bool, 7> keyReleased_{};
    mutable float wheelMove_ = 0.0f;
    mutable std::uint64_t frameCount_ = 0;
};

}  // namespace wb
