#pragma once

#include "render/Renderer.h"

namespace wb {

enum class MouseButton {
    Left,
    Right,
    Middle
};

enum class InputKey {
    Up,
    Down,
    Left,
    Right,
    Space,
    Enter,
    Escape
};

class InputProvider {
public:
    virtual ~InputProvider() = default;

    virtual void updateLogicalMapping(render::Rect viewport, render::Vec2 logicalSize) = 0;
    virtual render::Vec2 logicalMousePosition() const = 0;
    virtual render::Vec2 windowMousePosition() const = 0;
    virtual bool mouseInLogicalArea() const = 0;
    virtual bool mouseButtonDown(MouseButton button) const = 0;
    virtual bool mouseButtonPressed(MouseButton button) const = 0;
    virtual bool mouseButtonReleased(MouseButton button) const = 0;
    virtual bool keyDown(InputKey key) const = 0;
    virtual bool keyPressed(InputKey key) const = 0;
    virtual bool keyReleased(InputKey key) const = 0;
    virtual int typedCharacter() const = 0;
    virtual float scrollWheelMove() const = 0;
    virtual bool windowCloseRequested() const = 0;
};

}  // namespace wb
