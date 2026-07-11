#pragma once

#include "InputProvider.h"

namespace wb {

class RaylibInputProvider final : public InputProvider {
public:
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

private:
    render::Rect viewport_{};
    render::Vec2 logicalSize_{960.0f, 540.0f};
    render::Vec2 logicalMouse_{};
    render::Vec2 windowMouse_{};
    bool mouseInLogicalArea_ = false;
};

}  // namespace wb
