#include "render/vulkan/VulkanAppWindow.h"

#include "render/vulkan/VulkanBackend.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <thread>
#include <vector>

namespace wb {
namespace {

struct MonitorRecord {
    HMONITOR handle = nullptr;
    MONITORINFOEXW info{};
};

BOOL CALLBACK collectMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    auto* records = reinterpret_cast<std::vector<MonitorRecord>*>(data);
    MonitorRecord record;
    record.handle = monitor;
    record.info.cbSize = sizeof(record.info);
    if (GetMonitorInfoW(monitor, &record.info)) {
        records->push_back(record);
    }
    return TRUE;
}

std::vector<MonitorRecord> monitors() {
    std::vector<MonitorRecord> result;
    EnumDisplayMonitors(nullptr, nullptr, collectMonitor, reinterpret_cast<LPARAM>(&result));
    if (result.empty()) {
        MonitorRecord fallback;
        fallback.handle = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
        fallback.info.cbSize = sizeof(fallback.info);
        if (fallback.handle && GetMonitorInfoW(fallback.handle, &fallback.info)) {
            result.push_back(fallback);
        }
    }
    return result;
}

const MonitorRecord* monitorAt(const std::vector<MonitorRecord>& values, int index) {
    if (values.empty()) {
        return nullptr;
    }
    return &values[static_cast<std::size_t>(std::clamp(index, 0, static_cast<int>(values.size()) - 1))];
}

std::string utf8(const wchar_t* text) {
    if (!text || text[0] == L'\0') {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) {
        return {};
    }
    std::string value(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, value.data(), length, nullptr, nullptr);
    value.resize(static_cast<std::size_t>(length - 1));
    return value;
}

std::wstring wide(const char* text) {
    if (!text || text[0] == '\0') {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (length <= 1) {
        return {};
    }
    std::wstring value(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, value.data(), length);
    value.resize(static_cast<std::size_t>(length - 1));
    return value;
}

std::pair<int, int> clientSize(HWND window) {
    RECT client{};
    if (!window || !GetClientRect(window, &client)) {
        return {0, 0};
    }
    return {std::max(0L, client.right - client.left), std::max(0L, client.bottom - client.top)};
}

}  // namespace

VulkanAppWindow::VulkanAppWindow(void* instance, void* window, wbwwb::vulkan::VulkanBackend& backend)
    : instance_(instance), window_(window), backend_(backend) {
    const HWND hwnd = static_cast<HWND>(window_);
    RECT rect{};
    if (hwnd && GetWindowRect(hwnd, &rect)) {
        restoredX_ = rect.left;
        restoredY_ = rect.top;
        restoredWidth_ = rect.right - rect.left;
        restoredHeight_ = rect.bottom - rect.top;
    }
    if (hwnd) {
        restoredStyle_ = GetWindowLongPtrW(hwnd, GWL_STYLE);
    }
}

void VulkanAppWindow::initialize(const GameSettings&, const char* title) {
    const HWND hwnd = static_cast<HWND>(window_);
    if (!hwnd) {
        return;
    }
    const std::wstring windowTitle = wide(title);
    if (!windowTitle.empty()) {
        SetWindowTextW(hwnd, windowTitle.c_str());
    }
    initialized_ = true;
    lastFrame_ = std::chrono::steady_clock::now();
}

void VulkanAppWindow::shutdown() {
    initialized_ = false;
}

void VulkanAppWindow::applyIcons(const std::string&) {
    // The Vulkan Win32 class receives the multi-size icon from wbwwb.rc at creation time.
}

void VulkanAppWindow::setTargetFps(int fps) {
    targetFps_ = std::max(0, fps);
}

float VulkanAppWindow::frameTime() const {
    syncBackendToClient();
    auto now = std::chrono::steady_clock::now();
    if (targetFps_ > 0) {
        const auto target = std::chrono::microseconds(1000000 / targetFps_);
        const auto elapsed = now - lastFrame_;
        if (elapsed < target) {
            std::this_thread::sleep_for(target - elapsed);
            now = std::chrono::steady_clock::now();
        }
    }
    const float seconds = std::chrono::duration<float>(now - lastFrame_).count();
    lastFrame_ = now;
    return std::max(0.000001f, seconds);
}

void VulkanAppWindow::setVsync(bool enabled) {
    backend_.SetVsync(enabled);
}

int VulkanAppWindow::clientWidth() const {
    return std::max(1, clientSize(static_cast<HWND>(window_)).first);
}

int VulkanAppWindow::clientHeight() const {
    return std::max(1, clientSize(static_cast<HWND>(window_)).second);
}

int VulkanAppWindow::monitorCount() const {
    return std::max(1, static_cast<int>(monitors().size()));
}

int VulkanAppWindow::currentMonitor() const {
    const auto values = monitors();
    const HMONITOR current = MonitorFromWindow(static_cast<HWND>(window_), MONITOR_DEFAULTTONEAREST);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (values[i].handle == current) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

std::string VulkanAppWindow::monitorName(int index) const {
    const auto values = monitors();
    const MonitorRecord* record = monitorAt(values, index);
    if (!record) {
        return {};
    }
    DISPLAY_DEVICEW device{};
    device.cb = sizeof(device);
    if (EnumDisplayDevicesW(record->info.szDevice, 0, &device, 0) && device.DeviceString[0] != L'\0') {
        return utf8(device.DeviceString);
    }
    return utf8(record->info.szDevice);
}

int VulkanAppWindow::monitorWidth(int index) const {
    const auto values = monitors();
    const MonitorRecord* record = monitorAt(values, index);
    return record ? static_cast<int>(record->info.rcMonitor.right - record->info.rcMonitor.left) : 960;
}

int VulkanAppWindow::monitorHeight(int index) const {
    const auto values = monitors();
    const MonitorRecord* record = monitorAt(values, index);
    return record ? static_cast<int>(record->info.rcMonitor.bottom - record->info.rcMonitor.top) : 540;
}

render::Vec2 VulkanAppWindow::monitorPosition(int index) const {
    const auto values = monitors();
    const MonitorRecord* record = monitorAt(values, index);
    if (!record) {
        return {};
    }
    return {static_cast<float>(record->info.rcMonitor.left), static_cast<float>(record->info.rcMonitor.top)};
}

std::vector<WindowSizeOption> VulkanAppWindow::monitorWindowSizes() const {
    std::vector<WindowSizeOption> result;
    for (const MonitorRecord& monitor : monitors()) {
        result.push_back({
            static_cast<int>(monitor.info.rcMonitor.right - monitor.info.rcMonitor.left),
            static_cast<int>(monitor.info.rcMonitor.bottom - monitor.info.rcMonitor.top),
            {}
        });
    }
    return result;
}

bool VulkanAppWindow::isFullscreen() const {
    return fullscreen_;
}

void VulkanAppWindow::setUndecorated(bool enabled) {
    if (undecorated_ == enabled || fullscreen_) {
        return;
    }
    const HWND hwnd = static_cast<HWND>(window_);
    if (!hwnd) {
        return;
    }
    undecorated_ = enabled;
    const LONG_PTR style = enabled ? static_cast<LONG_PTR>(WS_POPUP | WS_VISIBLE)
                                   : static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    syncBackendToClient();
}

void VulkanAppWindow::setWindowPosition(int x, int y) {
    const HWND hwnd = static_cast<HWND>(window_);
    if (hwnd) {
        SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void VulkanAppWindow::setWindowSize(int width, int height) {
    const HWND hwnd = static_cast<HWND>(window_);
    if (!hwnd || width <= 0 || height <= 0) {
        return;
    }
    RECT rect{0, 0, width, height};
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    SetWindowPos(hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    syncBackendToClient();
}

void VulkanAppWindow::setFullscreen(bool enabled) {
    if (fullscreen_ == enabled) {
        return;
    }
    const HWND hwnd = static_cast<HWND>(window_);
    if (!hwnd) {
        return;
    }
    if (enabled) {
        RECT current{};
        GetWindowRect(hwnd, &current);
        restoredX_ = current.left;
        restoredY_ = current.top;
        restoredWidth_ = current.right - current.left;
        restoredHeight_ = current.bottom - current.top;
        restoredStyle_ = GetWindowLongPtrW(hwnd, GWL_STYLE);

        MONITORINFO monitor{};
        monitor.cbSize = sizeof(monitor);
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor);
        SetWindowLongPtrW(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hwnd, HWND_TOP,
                     monitor.rcMonitor.left,
                     monitor.rcMonitor.top,
                     monitor.rcMonitor.right - monitor.rcMonitor.left,
                     monitor.rcMonitor.bottom - monitor.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_NOACTIVATE);
        fullscreen_ = true;
    } else {
        const LONG_PTR style = restoredStyle_ != 0 ? restoredStyle_ : static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SetWindowPos(hwnd, nullptr, restoredX_, restoredY_, restoredWidth_, restoredHeight_,
                     SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
        fullscreen_ = false;
    }
    syncBackendToClient();
}

void VulkanAppWindow::syncBackendToClient() const {
    const HWND hwnd = static_cast<HWND>(window_);
    if (!hwnd || IsIconic(hwnd)) {
        return;
    }
    const auto [width, height] = clientSize(hwnd);
    if (width <= 0 || height <= 0) {
        return;
    }
    const auto stats = backend_.Stats();
    if (stats.swapchainExtent.width != static_cast<std::uint32_t>(width) ||
        stats.swapchainExtent.height != static_cast<std::uint32_t>(height)) {
        backend_.Resize(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    }
}

}  // namespace wb
