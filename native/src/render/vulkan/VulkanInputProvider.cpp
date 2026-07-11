#include "render/vulkan/VulkanInputProvider.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <utility>

namespace wb {
namespace {

std::size_t mouseIndex(MouseButton button) {
    switch (button) {
        case MouseButton::Right: return 1;
        case MouseButton::Middle: return 2;
        case MouseButton::Left:
        default: return 0;
    }
}

int mouseVirtualKey(std::size_t index) {
    static constexpr int keys[] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};
    return keys[std::min<std::size_t>(index, 2)];
}

std::size_t keyIndex(InputKey key) {
    return static_cast<std::size_t>(key);
}

int keyVirtualKey(std::size_t index) {
    static constexpr int keys[] = {VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_SPACE, VK_RETURN, VK_ESCAPE};
    return keys[std::min<std::size_t>(index, 6)];
}

bool virtualKeyDown(int key) {
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

}  // namespace

VulkanInputProvider::VulkanInputProvider(void* window,
                                         bool* shouldClose,
                                         float* wheelDelta,
                                         VulkanInputTestScript script)
    : window_(window), shouldClose_(shouldClose), wheelDelta_(wheelDelta), script_(std::move(script)) {
    for (std::size_t i = 0; i < mouseDown_.size(); ++i) {
        mouseDown_[i] = virtualKeyDown(mouseVirtualKey(i));
    }
    for (std::size_t i = 0; i < keyDown_.size(); ++i) {
        keyDown_[i] = virtualKeyDown(keyVirtualKey(i));
    }
}

void VulkanInputProvider::updateLogicalMapping(render::Rect viewport, render::Vec2 logicalSize) {
    viewport_ = viewport;
    logicalSize_ = logicalSize;

    POINT point{};
    GetCursorPos(&point);
    ScreenToClient(static_cast<HWND>(window_), &point);
    windowMouse_ = {static_cast<float>(point.x), static_cast<float>(point.y)};
    mouseInLogicalArea_ =
        windowMouse_.x >= viewport_.x &&
        windowMouse_.y >= viewport_.y &&
        windowMouse_.x <= viewport_.x + viewport_.width &&
        windowMouse_.y <= viewport_.y + viewport_.height;

    logicalMouse_.x = (windowMouse_.x - viewport_.x) * logicalSize_.x / std::max(1.0f, viewport_.width);
    logicalMouse_.y = (windowMouse_.y - viewport_.y) * logicalSize_.y / std::max(1.0f, viewport_.height);
    logicalMouse_.x = std::clamp(logicalMouse_.x, 0.0f, logicalSize_.x);
    logicalMouse_.y = std::clamp(logicalMouse_.y, 0.0f, logicalSize_.y);
    if (script_.overrideLogicalMouse) {
        logicalMouse_ = script_.logicalMouse;
        mouseInLogicalArea_ = true;
    }
}

render::Vec2 VulkanInputProvider::logicalMousePosition() const { return logicalMouse_; }
render::Vec2 VulkanInputProvider::windowMousePosition() const { return windowMouse_; }
bool VulkanInputProvider::mouseInLogicalArea() const { return mouseInLogicalArea_; }

bool VulkanInputProvider::mouseButtonDown(MouseButton button) const {
    return mouseDown_[mouseIndex(button)];
}

bool VulkanInputProvider::mouseButtonPressed(MouseButton button) const {
    return mousePressed_[mouseIndex(button)];
}

bool VulkanInputProvider::mouseButtonReleased(MouseButton button) const {
    return mouseReleased_[mouseIndex(button)];
}

bool VulkanInputProvider::keyDown(InputKey key) const {
    return keyDown_[keyIndex(key)];
}

bool VulkanInputProvider::keyPressed(InputKey key) const {
    return keyPressed_[keyIndex(key)];
}

bool VulkanInputProvider::keyReleased(InputKey key) const {
    return keyReleased_[keyIndex(key)];
}

int VulkanInputProvider::typedCharacter() const { return 0; }
float VulkanInputProvider::scrollWheelMove() const { return wheelMove_; }

bool VulkanInputProvider::windowCloseRequested() const {
    pollFrame();
    if (script_.maxFrames > 0 && frameCount_ > script_.maxFrames) {
        return true;
    }
    return (shouldClose_ && *shouldClose_) || !IsWindow(static_cast<HWND>(window_));
}

void VulkanInputProvider::pollFrame() const {
    ++frameCount_;

    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT && shouldClose_) {
            *shouldClose_ = true;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    for (std::size_t i = 0; i < mouseDown_.size(); ++i) {
        const bool previous = mouseDown_[i];
        mouseDown_[i] = virtualKeyDown(mouseVirtualKey(i));
        mousePressed_[i] = mouseDown_[i] && !previous;
        mouseReleased_[i] = !mouseDown_[i] && previous;
    }
    for (std::size_t i = 0; i < keyDown_.size(); ++i) {
        const bool previous = keyDown_[i];
        keyDown_[i] = virtualKeyDown(keyVirtualKey(i));
        keyPressed_[i] = keyDown_[i] && !previous;
        keyReleased_[i] = !keyDown_[i] && previous;
    }

    if (std::find(script_.enterFrames.begin(), script_.enterFrames.end(), frameCount_) != script_.enterFrames.end()) {
        keyPressed_[keyIndex(InputKey::Enter)] = true;
    }
    if (std::find(script_.downFrames.begin(), script_.downFrames.end(), frameCount_) != script_.downFrames.end()) {
        keyPressed_[keyIndex(InputKey::Down)] = true;
    }
    if (std::find(script_.rightFrames.begin(), script_.rightFrames.end(), frameCount_) != script_.rightFrames.end()) {
        keyPressed_[keyIndex(InputKey::Right)] = true;
    }
    if (std::find(script_.escapeFrames.begin(), script_.escapeFrames.end(), frameCount_) != script_.escapeFrames.end()) {
        keyPressed_[keyIndex(InputKey::Escape)] = true;
    }
    if (std::find(script_.leftClickFrames.begin(), script_.leftClickFrames.end(), frameCount_) != script_.leftClickFrames.end()) {
        mousePressed_[mouseIndex(MouseButton::Left)] = true;
    }

    wheelMove_ = wheelDelta_ ? *wheelDelta_ : 0.0f;
    if (wheelDelta_) {
        *wheelDelta_ = 0.0f;
    }
}

}  // namespace wb
