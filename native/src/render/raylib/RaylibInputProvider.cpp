#include "render/raylib/RaylibInputProvider.h"

#include "raylib.h"

#include <algorithm>

namespace wb {
namespace {

int toRaylibMouseButton(MouseButton button) {
    switch (button) {
        case MouseButton::Right: return MOUSE_BUTTON_RIGHT;
        case MouseButton::Middle: return MOUSE_BUTTON_MIDDLE;
        case MouseButton::Left:
        default:
            return MOUSE_BUTTON_LEFT;
    }
}

int toRaylibKey(InputKey key) {
    switch (key) {
        case InputKey::Up: return KEY_UP;
        case InputKey::Down: return KEY_DOWN;
        case InputKey::Left: return KEY_LEFT;
        case InputKey::Right: return KEY_RIGHT;
        case InputKey::Space: return KEY_SPACE;
        case InputKey::Enter: return KEY_ENTER;
        case InputKey::Escape: return KEY_ESCAPE;
    }
    return KEY_NULL;
}

}  // namespace

void RaylibInputProvider::updateLogicalMapping(render::Rect viewport, render::Vec2 logicalSize) {
    viewport_ = viewport;
    logicalSize_ = logicalSize;

    const Vector2 mouse = GetMousePosition();
    windowMouse_ = {mouse.x, mouse.y};
    mouseInLogicalArea_ =
        mouse.x >= viewport_.x &&
        mouse.y >= viewport_.y &&
        mouse.x <= viewport_.x + viewport_.width &&
        mouse.y <= viewport_.y + viewport_.height;

    logicalMouse_.x = (mouse.x - viewport_.x) * logicalSize_.x / std::max(1.0f, viewport_.width);
    logicalMouse_.y = (mouse.y - viewport_.y) * logicalSize_.y / std::max(1.0f, viewport_.height);
    logicalMouse_.x = std::clamp(logicalMouse_.x, 0.0f, logicalSize_.x);
    logicalMouse_.y = std::clamp(logicalMouse_.y, 0.0f, logicalSize_.y);
}

render::Vec2 RaylibInputProvider::logicalMousePosition() const {
    return logicalMouse_;
}

render::Vec2 RaylibInputProvider::windowMousePosition() const {
    return windowMouse_;
}

bool RaylibInputProvider::mouseInLogicalArea() const {
    return mouseInLogicalArea_;
}

bool RaylibInputProvider::mouseButtonDown(MouseButton button) const {
    return IsMouseButtonDown(toRaylibMouseButton(button));
}

bool RaylibInputProvider::mouseButtonPressed(MouseButton button) const {
    return IsMouseButtonPressed(toRaylibMouseButton(button));
}

bool RaylibInputProvider::mouseButtonReleased(MouseButton button) const {
    return IsMouseButtonReleased(toRaylibMouseButton(button));
}

bool RaylibInputProvider::keyDown(InputKey key) const {
    return IsKeyDown(toRaylibKey(key));
}

bool RaylibInputProvider::keyPressed(InputKey key) const {
    return IsKeyPressed(toRaylibKey(key));
}

bool RaylibInputProvider::keyReleased(InputKey key) const {
    return IsKeyReleased(toRaylibKey(key));
}

int RaylibInputProvider::typedCharacter() const {
    return GetCharPressed();
}

float RaylibInputProvider::scrollWheelMove() const {
    return GetMouseWheelMove();
}

bool RaylibInputProvider::windowCloseRequested() const {
    return WindowShouldClose();
}

}  // namespace wb
