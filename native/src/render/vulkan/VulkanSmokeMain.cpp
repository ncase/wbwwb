#ifndef _WIN32
#error "VulkanSmokeMain currently supports the Win32 surface path only."
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "AssetManager.h"
#include "Game.h"
#include "render/RenderBackend.h"
#include "render/miniaudio/MiniaudioAudioBackend.h"
#include "render/vulkan/VulkanAppWindow.h"
#include "render/vulkan/VulkanBackend.h"
#include "render/vulkan/VulkanCaptureService.h"
#include "render/vulkan/VulkanFramePipeline.h"
#include "render/vulkan/VulkanInputProvider.h"
#include "render/vulkan/VulkanRenderer.h"
#include "resource.h"
#include "Settings.h"

#include <windows.h>

#ifdef DrawText
#undef DrawText
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifndef WBWWB_VULKAN_SHADER_DIR
#define WBWWB_VULKAN_SHADER_DIR ""
#endif

namespace {

constexpr wchar_t kWindowClassName[] = L"WBWWBVulkanSmokeWindow";
constexpr int kSmokeWidth = 960;
constexpr int kSmokeHeight = 540;
bool g_shouldClose = false;
float g_mouseWheelDelta = 0.0f;

std::string lastWin32Error(const char* action) {
    const DWORD error = GetLastError();
    char* buffer = nullptr;
    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr);

    std::string message = std::string(action) + " failed";
    if (length > 0 && buffer != nullptr) {
        message += ": ";
        message += buffer;
    } else {
        message += " with Win32 error " + std::to_string(error);
    }

    if (buffer != nullptr) {
        LocalFree(buffer);
    }
    return message;
}

bool hasArg(int argc, char** argv, const std::string& needle) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && needle == argv[i]) {
            return true;
        }
    }
    return false;
}

std::filesystem::path executableDirectory() {
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (length == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (length == 0) {
        throw std::runtime_error(lastWin32Error("GetModuleFileNameW"));
    }
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
}

std::filesystem::path shaderDirectory() {
    const std::filesystem::path local = executableDirectory() / "vulkan-shaders";
    if (std::filesystem::exists(local / "solid2d.vert.spv") &&
        std::filesystem::exists(local / "solid2d.frag.spv")) {
        return local;
    }
    return std::filesystem::path(WBWWB_VULKAN_SHADER_DIR);
}

std::optional<std::filesystem::path> findAssetRoot() {
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(std::filesystem::current_path());
    candidates.push_back(executableDirectory());

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        std::filesystem::path current = candidates[i];
        while (!current.empty()) {
            if (std::filesystem::exists(current / "sprites" / "peeps" / "body.png") &&
                std::filesystem::exists(current / "sounds")) {
                return current;
            }
            const std::filesystem::path parent = current.parent_path();
            if (parent == current) {
                break;
            }
            current = parent;
        }
    }

    return std::nullopt;
}

std::filesystem::path findFontPath() {
    const std::array<std::filesystem::path, 6> candidates{{
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf"
    }};
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CLOSE:
        g_shouldClose = true;
        PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        g_shouldClose = true;
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEWHEEL:
        g_mouseWheelDelta += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

HWND createSmokeWindow(HINSTANCE instance, int width, int height) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_WBWWB), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
    windowClass.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_WBWWB), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
    windowClass.lpszClassName = kWindowClassName;

    if (RegisterClassExW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        throw std::runtime_error(lastWin32Error("RegisterClassExW"));
    }

    const DWORD style = WS_OVERLAPPEDWINDOW;
    RECT windowRect{0, 0, width, height};
    if (!AdjustWindowRect(&windowRect, style, FALSE)) {
        throw std::runtime_error(lastWin32Error("AdjustWindowRect"));
    }

    HWND hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        L"Native WBWWB C++ Port - Vulkan Smoke Test",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (hwnd == nullptr) {
        throw std::runtime_error(lastWin32Error("CreateWindowExW"));
    }

    return hwnd;
}

void resizeClientWindow(HWND hwnd, int width, int height) {
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    RECT windowRect{0, 0, width, height};
    if (!AdjustWindowRect(&windowRect, style, FALSE)) {
        throw std::runtime_error(lastWin32Error("AdjustWindowRect"));
    }
    SetWindowPos(hwnd,
                 nullptr,
                 0,
                 0,
                 windowRect.right - windowRect.left,
                 windowRect.bottom - windowRect.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

struct LogicalInput {
    float x = 0.0f;
    float y = 0.0f;
    bool inside = false;
    bool leftDown = false;
    bool leftPressed = false;
    float wheel = 0.0f;
};

struct KeyEdges {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool enter = false;
    bool space = false;
    bool escape = false;
    bool backspace = false;
};

class InputTracker final {
public:
    LogicalInput Mouse(HWND hwnd, const VkRect2D& presentationRect) {
        POINT point{};
        GetCursorPos(&point);
        ScreenToClient(hwnd, &point);
        const bool leftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        LogicalInput input;
        input.leftDown = leftDown;
        input.leftPressed = leftDown && !previousLeftDown_;
        previousLeftDown_ = leftDown;
        input.wheel = g_mouseWheelDelta;
        g_mouseWheelDelta = 0.0f;

        const float px = static_cast<float>(point.x);
        const float py = static_cast<float>(point.y);
        const float rectX = static_cast<float>(presentationRect.offset.x);
        const float rectY = static_cast<float>(presentationRect.offset.y);
        const float rectW = static_cast<float>(std::max<std::uint32_t>(1, presentationRect.extent.width));
        const float rectH = static_cast<float>(std::max<std::uint32_t>(1, presentationRect.extent.height));
        input.inside = px >= rectX && py >= rectY && px <= rectX + rectW && py <= rectY + rectH;
        input.x = std::clamp((px - rectX) * 960.0f / rectW, 0.0f, 960.0f);
        input.y = std::clamp((py - rectY) * 540.0f / rectH, 0.0f, 540.0f);
        return input;
    }

    KeyEdges Keys() {
        KeyEdges edges;
        edges.up = Pressed(VK_UP);
        edges.down = Pressed(VK_DOWN);
        edges.left = Pressed(VK_LEFT);
        edges.right = Pressed(VK_RIGHT);
        edges.enter = Pressed(VK_RETURN);
        edges.space = Pressed(VK_SPACE);
        edges.escape = Pressed(VK_ESCAPE);
        edges.backspace = Pressed(VK_BACK);
        return edges;
    }

private:
    bool Pressed(int key) {
        const bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
        const bool pressed = down && !previousKeys_[static_cast<std::size_t>(key)];
        previousKeys_[static_cast<std::size_t>(key)] = down;
        return pressed;
    }

    bool previousLeftDown_ = false;
    std::array<bool, 256> previousKeys_{};
};

bool pointIn(float x, float y, float rx, float ry, float rw, float rh) {
    return x >= rx && y >= ry && x <= rx + rw && y <= ry + rh;
}

int frameLimitFps(int value) {
    switch (value) {
        case 0: return 30;
        case 2: return 120;
        case 3: return 0;
        default: return 60;
    }
}

const char* onOff(bool value) {
    return value ? "settings.value.on" : "settings.value.off";
}

const char* outputLabel(int value) {
    return value == 1 ? "settings.value.linear" : "settings.value.nearest";
}

std::string msaaLabel(int value) {
    switch (value) {
        case 2: return "2x";
        case 4: return "4x";
        case 8: return "8x";
        case 16: return "16x";
        default: return "Off";
    }
}

std::string frameLimitLabel(int value) {
    switch (value) {
        case 0: return "30 FPS";
        case 2: return "120 FPS";
        case 3: return "Unlimited";
        default: return "60 FPS";
    }
}

std::string percentLabel(float value) {
    const int percent = static_cast<int>(std::round(std::clamp(value, 0.0f, 1.0f) * 100.0f));
    return std::to_string(percent) + "%";
}

enum class VulkanScene {
    Menu,
    Settings,
    SharedGameBoundary
};

enum class VulkanSettingsRowKind {
    Section,
    MasterVolume,
    MusicVolume,
    SfxVolume,
    Language,
    WindowSize,
    Vsync,
    OutputSmoothing,
    Msaa,
    Fxaa,
    FrameLimit,
    DisplayMode,
    Reset,
    Back
};

struct VulkanSettingsRowDef {
    VulkanSettingsRowKind kind;
    const char* labelKey;
    std::string value;
    bool valueIsKey = false;
    bool slider = false;
    float sliderValue = 0.0f;
    bool enabled = true;
};

struct VulkanMenuSceneState {
    float playHoverScale = 1.0f;
    float settingsHoverScale = 1.0f;
    int selected = 0;
};

struct VulkanSettingsSceneState {
    int section = 0;
    int selected = 0;
    int dragging = -1;
    float scrollOffset = 0.0f;
};

std::vector<VulkanSettingsRowDef> buildSettingsRows(const wb::GameSettings& settings, int section) {
    std::vector<VulkanSettingsRowDef> rows;
    rows.push_back({VulkanSettingsRowKind::Section, "settings.category", section == 0 ? "settings.tab.audio" : section == 1 ? "settings.tab.display" : section == 2 ? "settings.tab.visuals" : "settings.tab.monitor", true});
    if (section == 0) {
        rows.push_back({VulkanSettingsRowKind::MasterVolume, "settings.master_volume", percentLabel(settings.masterVolume), false, true, settings.masterVolume});
        rows.push_back({VulkanSettingsRowKind::MusicVolume, "settings.music_volume", percentLabel(settings.musicVolume), false, true, settings.musicVolume});
        rows.push_back({VulkanSettingsRowKind::SfxVolume, "settings.sfx_volume", percentLabel(settings.sfxVolume), false, true, settings.sfxVolume});
    } else if (section == 1) {
        rows.push_back({VulkanSettingsRowKind::Language, "settings.language", "English", false, false, 0.0f, false});
        rows.push_back({VulkanSettingsRowKind::WindowSize, "settings.window_size", settings.currentWindowSize().label});
        rows.push_back({VulkanSettingsRowKind::Vsync, "settings.vsync", onOff(settings.vsync), true});
        rows.push_back({VulkanSettingsRowKind::OutputSmoothing, "settings.output_smoothing", outputLabel(settings.outputSmoothing), true});
    } else if (section == 2) {
        rows.push_back({VulkanSettingsRowKind::Msaa, "settings.msaa", msaaLabel(settings.msaaSamples), false, false, 0.0f, false});
        rows.push_back({VulkanSettingsRowKind::Fxaa, "settings.fxaa", onOff(settings.fxaa), true, false, 0.0f, false});
        rows.push_back({VulkanSettingsRowKind::Reset, "settings.reset", ""});
    } else {
        rows.push_back({VulkanSettingsRowKind::DisplayMode, "settings.display_mode", "Vulkan pending", false, false, 0.0f, false});
        rows.push_back({VulkanSettingsRowKind::FrameLimit, "settings.frame_limit", frameLimitLabel(settings.frameLimit)});
    }
    rows.push_back({VulkanSettingsRowKind::Back, "settings.back", "settings.value.esc", true});
    return rows;
}

float settingsScreenX() { return 480.0f - 300.0f * 1.6f / 2.0f + 29.0f * 1.6f + 10.0f; }
float settingsScreenY() { return 570.0f - 360.0f * 1.6f + 66.0f * 1.6f + 9.0f; }
float settingsScreenW() { return 240.0f * 1.6f - 20.0f; }
float settingsScreenH() { return 137.0f * 1.6f - 18.0f; }
float settingsRowsX() { return settingsScreenX() + 10.0f; }
float settingsRowsY() { return settingsScreenY() + 48.0f; }
float settingsRowsW() { return settingsScreenW() - 20.0f; }
float settingsRowsH() { return settingsScreenH() - 56.0f; }
float settingsRowPitch() { return 16.0f; }

float maxSettingsScroll(std::size_t rowCount) {
    return std::max(0.0f, static_cast<float>(rowCount) * settingsRowPitch() - settingsRowsH());
}

void clampSettingsSelection(VulkanSettingsSceneState& state, const std::vector<VulkanSettingsRowDef>& rows) {
    state.selected = std::clamp(state.selected, 0, std::max(0, static_cast<int>(rows.size()) - 1));
    state.scrollOffset = std::clamp(state.scrollOffset, 0.0f, maxSettingsScroll(rows.size()));
}

void ensureSettingsSelectedVisible(VulkanSettingsSceneState& state, const std::vector<VulkanSettingsRowDef>& rows) {
    const float rowTop = static_cast<float>(state.selected) * settingsRowPitch();
    const float rowBottom = rowTop + 14.0f;
    if (rowTop < state.scrollOffset) {
        state.scrollOffset = rowTop;
    } else if (rowBottom > state.scrollOffset + settingsRowsH()) {
        state.scrollOffset = rowBottom - settingsRowsH();
    }
    state.scrollOffset = std::clamp(state.scrollOffset, 0.0f, maxSettingsScroll(rows.size()));
}

int hoveredSettingsRow(const LogicalInput& input, const VulkanSettingsSceneState& state, const std::vector<VulkanSettingsRowDef>& rows) {
    if (!input.inside || !pointIn(input.x, input.y, settingsRowsX(), settingsRowsY(), settingsRowsW(), settingsRowsH())) {
        return -1;
    }
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const float rowY = settingsRowsY() + static_cast<float>(i) * settingsRowPitch() - state.scrollOffset;
        if (pointIn(input.x, input.y, settingsRowsX(), rowY, settingsRowsW(), 14.0f)) {
            return i;
        }
    }
    return -1;
}

void setSettingSlider(wb::GameSettings& settings, VulkanSettingsRowKind kind, float mouseX) {
    const float sliderX = settingsRowsX() + settingsRowsW() - 108.0f;
    const float value = std::clamp((mouseX - sliderX) / 94.0f, 0.0f, 1.0f);
    if (kind == VulkanSettingsRowKind::MasterVolume) {
        settings.masterVolume = value;
    } else if (kind == VulkanSettingsRowKind::MusicVolume) {
        settings.musicVolume = value;
    } else if (kind == VulkanSettingsRowKind::SfxVolume) {
        settings.sfxVolume = value;
    }
}

void applyVulkanPresentationSettings(wbwwb::vulkan::VulkanBackend& backend, const wb::GameSettings& settings) {
    backend.SetPresentationOptions(settings.vsync, settings.outputSmoothing == 1);
}

void drawPipelineBoundaryScene(wb::render::Renderer& renderer, wb::render::FontHandle font) {
    renderer.DrawRectangle({90.0f, 118.0f, 780.0f, 306.0f}, {18, 18, 20, 238});
    renderer.DrawRectangleOutline({90.0f, 118.0f, 780.0f, 306.0f}, 3.0f, {184, 184, 184, 218});
    renderer.DrawRectangle({90.0f, 118.0f, 780.0f, 52.0f}, {214, 32, 32, 235});
    renderer.DrawText({font, "VULKAN FRAME PIPELINE", {252.0f, 132.0f}, 31.0f, {255, 255, 255, 255}, 1.0f});
    renderer.DrawText({font, "Shared Game and SceneManager now run through Vulkan.", {164.0f, 206.0f}, 23.0f, {245, 245, 245, 255}, 1.0f});
    renderer.DrawText({font, "Gameplay uses Win32 input and miniaudio playback.", {142.0f, 262.0f}, 18.0f, {214, 214, 214, 255}, 1.0f});
    renderer.DrawText({font, "Camera and TV photos use Vulkan render-target capture.", {142.0f, 288.0f}, 18.0f, {214, 214, 214, 255}, 1.0f});
    renderer.DrawText({font, "The boundary test exercises the real gameplay path.", {142.0f, 344.0f}, 18.0f, {198, 198, 198, 255}, 1.0f});
    renderer.DrawText({font, "Esc / Backspace returns to menu.", {326.0f, 390.0f}, 18.0f, {245, 245, 245, 255}, 1.0f});
}

void resizeVulkanWindowToSettings(HWND hwnd, wbwwb::vulkan::VulkanBackend& backend, const wb::GameSettings& settings) {
    const wb::WindowSizeOption& window = settings.currentWindowSize();
    resizeClientWindow(hwnd, window.width, window.height);
    backend.Resize(static_cast<std::uint32_t>(window.width), static_cast<std::uint32_t>(window.height));
}

bool activateSettingsRow(HWND hwnd,
                         wbwwb::vulkan::VulkanBackend& backend,
                         wb::GameSettings& settings,
                         VulkanSettingsSceneState& state,
                         const std::vector<VulkanSettingsRowDef>& rows,
                         int rowIndex) {
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size())) {
        return false;
    }
    const VulkanSettingsRowDef& row = rows[static_cast<std::size_t>(rowIndex)];
    if (!row.enabled && row.kind != VulkanSettingsRowKind::Back) {
        return false;
    }
    switch (row.kind) {
        case VulkanSettingsRowKind::Section:
            state.section = (state.section + 1) % 4;
            state.selected = 0;
            state.scrollOffset = 0.0f;
            return false;
        case VulkanSettingsRowKind::Vsync:
            settings.vsync = !settings.vsync;
            settings.save();
            applyVulkanPresentationSettings(backend, settings);
            return false;
        case VulkanSettingsRowKind::OutputSmoothing:
            settings.outputSmoothing = settings.outputSmoothing == 1 ? 0 : 1;
            settings.save();
            applyVulkanPresentationSettings(backend, settings);
            return false;
        case VulkanSettingsRowKind::WindowSize:
            settings.stepWindowSize(1);
            settings.save();
            resizeVulkanWindowToSettings(hwnd, backend, settings);
            return false;
        case VulkanSettingsRowKind::FrameLimit:
            settings.stepFrameLimit(1);
            settings.save();
            return false;
        case VulkanSettingsRowKind::Reset:
            settings.resetDefaults();
            settings.save();
            applyVulkanPresentationSettings(backend, settings);
            resizeVulkanWindowToSettings(hwnd, backend, settings);
            state.section = 0;
            state.selected = 0;
            state.scrollOffset = 0.0f;
            return false;
        case VulkanSettingsRowKind::Back:
            settings.save();
            return true;
        default:
            return false;
    }
}

void adjustSettingsRow(HWND hwnd,
                       wbwwb::vulkan::VulkanBackend& backend,
                       wb::GameSettings& settings,
                       VulkanSettingsSceneState& state,
                       const std::vector<VulkanSettingsRowDef>& rows,
                       int direction) {
    if (state.selected < 0 || state.selected >= static_cast<int>(rows.size())) {
        return;
    }
    const VulkanSettingsRowKind kind = rows[static_cast<std::size_t>(state.selected)].kind;
    if (kind == VulkanSettingsRowKind::Section) {
        state.section = (state.section + direction + 4) % 4;
        state.selected = 0;
        state.scrollOffset = 0.0f;
    } else if (kind == VulkanSettingsRowKind::MasterVolume) {
        settings.masterVolume = std::clamp(settings.masterVolume + 0.05f * static_cast<float>(direction), 0.0f, 1.0f);
        settings.save();
    } else if (kind == VulkanSettingsRowKind::MusicVolume) {
        settings.musicVolume = std::clamp(settings.musicVolume + 0.05f * static_cast<float>(direction), 0.0f, 1.0f);
        settings.save();
    } else if (kind == VulkanSettingsRowKind::SfxVolume) {
        settings.sfxVolume = std::clamp(settings.sfxVolume + 0.05f * static_cast<float>(direction), 0.0f, 1.0f);
        settings.save();
    } else if (kind == VulkanSettingsRowKind::WindowSize) {
        settings.stepWindowSize(direction);
        settings.save();
        resizeVulkanWindowToSettings(hwnd, backend, settings);
    } else if (kind == VulkanSettingsRowKind::FrameLimit) {
        settings.stepFrameLimit(direction);
        settings.save();
    } else if (kind == VulkanSettingsRowKind::Vsync || kind == VulkanSettingsRowKind::OutputSmoothing) {
        activateSettingsRow(hwnd, backend, settings, state, rows, state.selected);
    }
}

wbwwb::vulkan::VulkanUiRuntimeState makeVulkanUiState(VulkanScene scene,
                                                       const VulkanMenuSceneState& menu,
                                                       const VulkanSettingsSceneState& settingsState,
                                                       const wb::GameSettings& settings,
                                                       const LogicalInput& input) {
    wbwwb::vulkan::VulkanUiRuntimeState state;
    if (scene == VulkanScene::Menu) {
        state.mode = wbwwb::vulkan::VulkanUiTestMode::Menu;
        state.menu.playHoverScale = menu.playHoverScale;
        state.menu.settingsHoverScale = menu.settingsHoverScale;
        state.menu.selected = menu.selected;
        state.menu.playHovered = input.inside && pointIn(input.x, input.y, 80.5f, 207.5f, 395.0f, 85.0f);
        state.menu.settingsHovered = input.inside && pointIn(input.x, input.y, 716.0f, 394.0f, 154.0f, 58.0f);
    } else if (scene == VulkanScene::Settings) {
        state.mode = wbwwb::vulkan::VulkanUiTestMode::Settings;
        state.settings.section = settingsState.section;
        state.settings.scrollOffset = settingsState.scrollOffset;
        const std::vector<VulkanSettingsRowDef> rows = buildSettingsRows(settings, settingsState.section);
        const int hovered = hoveredSettingsRow(input, settingsState, rows);
        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            const VulkanSettingsRowDef& row = rows[static_cast<std::size_t>(i)];
            state.settings.rows.push_back({
                row.labelKey,
                row.value,
                row.valueIsKey,
                row.slider,
                row.sliderValue,
                i == settingsState.selected,
                i == hovered,
                row.enabled
            });
        }
    } else {
        state.mode = wbwwb::vulkan::VulkanUiTestMode::SharedGameBoundary;
    }
    return state;
}

} // namespace

int main(int argc, char** argv) {
    const bool smokeTest = hasArg(argc, argv, "--smoke-test");
    const bool smokeGameRender = hasArg(argc, argv, "--smoke-game-render");
    const bool menuTest = hasArg(argc, argv, "--vulkan-menu-test");
    const bool settingsTest = hasArg(argc, argv, "--vulkan-settings-test");
    const bool rendererContractTest = hasArg(argc, argv, "--vulkan-renderer-test");
    const bool boundaryTest = hasArg(argc, argv, "--vulkan-boundary-test");
    const bool pauseTest = hasArg(argc, argv, "--vulkan-pause-test");
    const bool captureTest = hasArg(argc, argv, "--vulkan-capture-test");
    const bool hdrTest = hasArg(argc, argv, "--vulkan-hdr-test");
    const bool hdrAutoTest = hasArg(argc, argv, "--vulkan-hdr-auto-test");
    const bool msaaTest = hasArg(argc, argv, "--vulkan-msaa-test");
    const bool resolutionTest = hasArg(argc, argv, "--vulkan-resolution-test");
    const bool automatedTest = smokeTest || menuTest || settingsTest || rendererContractTest || boundaryTest || pauseTest || captureTest || hdrTest || hdrAutoTest || msaaTest || resolutionTest;
    const bool normalLaunch = !automatedTest && !smokeGameRender;
    HINSTANCE instance = GetModuleHandleW(nullptr);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    try {
        wb::GameSettings settings;
        if (normalLaunch) {
            settings.load();
            settings.refreshWindowSizeOptions(wb::detectSystemWindowSizeOptions());
        }

        const wb::WindowSizeOption launchWindow = normalLaunch
            ? settings.currentWindowSize()
            : wb::WindowSizeOption{kSmokeWidth, kSmokeHeight, "960 x 540"};
        HWND hwnd = createSmokeWindow(instance, launchWindow.width, launchWindow.height);
        if (normalLaunch) {
            SetWindowTextW(hwnd, L"Native WBWWB C++ Port");
        }
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        wbwwb::vulkan::VulkanBackendConfig config;
        config.applicationName = normalLaunch
            ? "Native WBWWB C++ Port"
            : "Native WBWWB C++ Port - Vulkan Smoke Test";
        config.windowWidth = static_cast<std::uint32_t>(launchWindow.width);
        config.windowHeight = static_cast<std::uint32_t>(launchWindow.height);
        config.renderWidth = resolutionTest ? 3840u : static_cast<std::uint32_t>(launchWindow.width);
        config.renderHeight = resolutionTest ? 2160u : static_cast<std::uint32_t>(launchWindow.height);
        config.win32Hinstance = instance;
        config.win32Hwnd = hwnd;
        config.clearColor = {0.025f, 0.025f, 0.03f, 1.0f};
        config.drawSmokeGeometry = !normalLaunch && !menuTest && !settingsTest && !rendererContractTest && !boundaryTest && !pauseTest && !captureTest && !msaaTest && !resolutionTest;
        config.drawGameAssetSmoke = smokeGameRender;
        config.vsync = normalLaunch ? settings.vsync : true;
        config.hdrMode = hdrTest || (normalLaunch && settings.hdrMode >= 2)
            ? wbwwb::vulkan::VulkanHdrMode::On
            : hdrAutoTest || (normalLaunch && settings.hdrMode == 1)
                ? wbwwb::vulkan::VulkanHdrMode::Auto
                : wbwwb::vulkan::VulkanHdrMode::Off;
        config.outputLinearFilter = normalLaunch ? settings.outputSmoothing == 1 : true;
        config.msaaSamples = msaaTest
            ? 16u
            : normalLaunch ? static_cast<std::uint32_t>(std::max(1, settings.msaaSamples)) : 1u;
        config.shaderDirectory = shaderDirectory();
        if (menuTest) {
            config.uiTestMode = wbwwb::vulkan::VulkanUiTestMode::Menu;
        } else if (settingsTest) {
            config.uiTestMode = wbwwb::vulkan::VulkanUiTestMode::Settings;
        }
        if (normalLaunch || smokeGameRender || menuTest || settingsTest || boundaryTest || pauseTest || resolutionTest) {
            const std::optional<std::filesystem::path> assetRoot = findAssetRoot();
            if (!assetRoot.has_value()) {
                throw std::runtime_error("Could not find WBWWB asset root for Vulkan render test");
            }
            config.assetRoot = *assetRoot;
            config.localizationPath = *assetRoot / "native" / "assets" / "lang" / "en.json";
            config.fontPath = findFontPath();
            if (smokeGameRender) {
                config.smokeTexturePath = *assetRoot / "sprites" / "peeps" / "body.png";
            }
        }
        if (rendererContractTest && config.fontPath.empty()) {
            config.fontPath = findFontPath();
        }
#ifdef NDEBUG
        config.enableValidation = false;
#else
        config.enableValidation = true;
#endif

        wbwwb::vulkan::VulkanBackend backend;
        backend.Initialize(config);
        if (hdrTest || hdrAutoTest) {
            wb::render::PostProcessSettings hdrCalibration;
            hdrCalibration.hdrPaperWhiteNits = 250.0f;
            hdrCalibration.hdrPeakNits = 1400.0f;
            hdrCalibration.hdrHighlightStrength = 0.6f;
            backend.SetPostProcessSettings(hdrCalibration);
        }

        const bool useVulkanFramePipeline = rendererContractTest || boundaryTest || pauseTest || captureTest || msaaTest || resolutionTest || normalLaunch;
        std::unique_ptr<wb::VulkanRenderer> rendererContract;
        std::unique_ptr<wb::VulkanFramePipeline> framePipeline;
        wb::render::TextureHandle contractTexture{};
        wb::render::FontHandle contractFont{};
        wb::render::RenderTargetHandle captureTarget{};
        wb::render::TextureHandle captureTexture{};
        wb::render::RenderTargetHandle resolutionProbeTarget{};
        wb::render::TextureHandle resolutionPicture{};
        std::unique_ptr<wb::AssetManager> resolutionAssets;
        if (useVulkanFramePipeline) {
            rendererContract = std::make_unique<wb::VulkanRenderer>(backend);
            framePipeline = std::make_unique<wb::VulkanFramePipeline>(*rendererContract);
            if (!normalLaunch && !boundaryTest && !pauseTest) {
                wb::render::RendererConfig rendererConfig;
                rendererConfig.logicalWidth = 960;
                rendererConfig.logicalHeight = 540;
                rendererConfig.renderWidth = resolutionTest ? 3840u : static_cast<std::uint32_t>(launchWindow.width);
                rendererConfig.renderHeight = resolutionTest ? 2160u : static_cast<std::uint32_t>(launchWindow.height);
                rendererConfig.outputWidth = static_cast<std::uint32_t>(launchWindow.width);
                rendererConfig.outputHeight = static_cast<std::uint32_t>(launchWindow.height);
                rendererConfig.vsync = true;
                rendererConfig.outputFilter = wb::render::TextureFilter::Linear;
                rendererConfig.requestedOutputColorMode = hdrTest
                    ? wb::render::OutputColorMode::Hdr
                    : wb::render::OutputColorMode::Sdr;
                rendererConfig.msaaSamples = msaaTest ? 16u : 1u;
                rendererContract->Initialize(rendererConfig);
            }
            if (resolutionTest) {
                const wb::render::RendererStats rendererStats = rendererContract->Stats();
                if (rendererStats.renderWidth != 3840u || rendererStats.renderHeight != 2160u) {
                    throw std::runtime_error("Vulkan renderer contract did not retain the selected 3840x2160 render resolution");
                }
                resolutionProbeTarget = rendererContract->CreateRenderTarget(rendererStats.renderWidth, rendererStats.renderHeight);
                const wb::render::TextureHandle probeTexture = rendererContract->RenderTargetTexture(resolutionProbeTarget);
                const wb::render::Vec2 probeSize = rendererContract->TextureSize(probeTexture);
                if (resolutionProbeTarget.id == 0 || probeTexture.id == 0 ||
                    static_cast<std::uint32_t>(std::lround(probeSize.x)) != 3840u ||
                    static_cast<std::uint32_t>(std::lround(probeSize.y)) != 2160u) {
                    throw std::runtime_error("Vulkan selected-resolution capture target was not 3840x2160");
                }
                resolutionAssets = std::make_unique<wb::AssetManager>(config.assetRoot);
                resolutionAssets->setRenderer(rendererContract.get());
                resolutionAssets->setSmoothTextures(true);
                resolutionAssets->updateRenderResolution(3840u, 2160u);
                resolutionPicture = resolutionAssets->textureHandle("sprites/bg_preload.png");
                const wb::render::Vec2 pictureSize = rendererContract->TextureSize(resolutionPicture);
                if (resolutionPicture.id == 0 ||
                    static_cast<std::uint32_t>(std::lround(pictureSize.x)) != 3840u ||
                    static_cast<std::uint32_t>(std::lround(pictureSize.y)) != 2160u) {
                    throw std::runtime_error("Vulkan render-resolution picture was not decoded at 3840x2160");
                }
            }
            if (rendererContractTest) {
                contractFont = rendererContract->LoadFont(config.fontPath, 48);
            }

            if (rendererContractTest) {
                const std::array<std::uint8_t, 16> pixels{{
                    214, 32, 32, 255, 246, 246, 246, 255,
                    246, 246, 246, 255, 214, 32, 32, 255
                }};
                contractTexture = rendererContract->CreateTextureRGBA({2, 2, wb::render::TextureFilter::Nearest},
                                                                      pixels.data());
            }
            if (captureTest) {
                constexpr std::uint32_t captureWidth = 128;
                constexpr std::uint32_t captureHeight = 64;
                captureTarget = rendererContract->CreateRenderTarget(captureWidth, captureHeight);
                captureTexture = rendererContract->RenderTargetTexture(captureTarget);
                rendererContract->BeginRenderTarget(captureTarget);
                rendererContract->DrawRectangle({0.0f, 0.0f, static_cast<float>(captureWidth), static_cast<float>(captureHeight)},
                                                {12, 18, 24, 255});
                rendererContract->DrawRectangle({16.0f, 12.0f, 96.0f, 40.0f}, {214, 32, 32, 255});
                rendererContract->DrawLine({16.0f, 12.0f}, {112.0f, 52.0f}, 3.0f, {255, 255, 255, 255});
                rendererContract->EndRenderTarget();

                const std::vector<std::uint8_t> pixels = rendererContract->CaptureRenderTargetRGBA(captureTarget);
                constexpr std::uint32_t sampleX = 64;
                constexpr std::uint32_t sampleY = 20;
                const std::size_t center = (sampleY * captureWidth + sampleX) * 4u;
                const bool sizeOk = pixels.size() == static_cast<std::size_t>(captureWidth) * captureHeight * 4u;
                const bool centerOk = sizeOk &&
                    pixels[center + 0] > 150 &&
                    pixels[center + 1] < 120 &&
                    pixels[center + 2] < 120 &&
                    pixels[center + 3] > 200;
                if (!centerOk) {
                    throw std::runtime_error("Vulkan capture test readback did not return the expected RGBA center pixel");
                }

                const std::array<std::uint8_t, 16> qualitySource{{
                    0, 0, 0, 255, 255, 255, 255, 255,
                    0, 0, 0, 255, 255, 255, 255, 255
                }};
                const wb::render::TextureHandle qualityTexture = rendererContract->CreateTextureRGBA(
                    {2, 2, wb::render::TextureFilter::Nearest},
                    qualitySource.data()
                );
                rendererContract->BeginRenderTarget(captureTarget);
                rendererContract->DrawTexture({
                    qualityTexture,
                    {0.0f, 0.0f, 2.0f, 2.0f},
                    {0.0f, 0.0f, static_cast<float>(captureWidth), static_cast<float>(captureHeight)},
                    {0.0f, 0.0f},
                    0.0f,
                    {255, 255, 255, 255},
                    1.0f,
                    false,
                    false
                });
                rendererContract->EndRenderTarget();

                const std::vector<std::uint8_t> qualityPixels = rendererContract->CaptureRenderTargetRGBA(captureTarget);
                constexpr std::uint32_t qualitySampleX = 63;
                constexpr std::uint32_t qualitySampleY = captureHeight / 2;
                const std::size_t qualitySample = (qualitySampleY * captureWidth + qualitySampleX) * 4u;
                const bool qualitySampleOk = qualityPixels.size() == pixels.size() &&
                    qualityPixels[qualitySample + 0] > 32 && qualityPixels[qualitySample + 0] < 223 &&
                    qualityPixels[qualitySample + 1] == qualityPixels[qualitySample + 0] &&
                    qualityPixels[qualitySample + 2] == qualityPixels[qualitySample + 0] &&
                    qualityPixels[qualitySample + 3] > 250;
                if (!qualitySampleOk) {
                    throw std::runtime_error("Vulkan universal high-quality sampling did not reconstruct a nearest-filtered texture");
                }

                const wb::render::RenderTargetHandle fsrSourceTarget = rendererContract->CreateRenderTarget(64, 32);
                rendererContract->BeginRenderTarget(fsrSourceTarget);
                rendererContract->DrawRectangle({0.0f, 0.0f, 64.0f, 32.0f}, {12, 18, 24, 255});
                rendererContract->DrawRectangle({8.0f, 6.0f, 48.0f, 20.0f}, {214, 32, 32, 255});
                rendererContract->DrawTexture({
                    qualityTexture,
                    {0.0f, 0.0f, 2.0f, 2.0f},
                    {8.0f, 8.0f, 48.0f, 16.0f},
                    {0.0f, 0.0f},
                    0.0f,
                    {255, 255, 255, 255},
                    1.0f,
                    false,
                    false
                });
                rendererContract->EndRenderTarget();
                if (!rendererContract->UpscaleRenderTargetFsr1(fsrSourceTarget, captureTarget, 1.0f)) {
                    throw std::runtime_error("Vulkan FSR 1 capture upscaling did not run");
                }
                const std::vector<std::uint8_t> fsrPixels = rendererContract->CaptureRenderTargetRGBA(captureTarget);
                constexpr std::uint32_t fsrSampleX = 63;
                constexpr std::uint32_t fsrSampleY = captureHeight / 2;
                const std::size_t fsrSample = (fsrSampleY * captureWidth + fsrSampleX) * 4u;
                const bool fsrSampleOk = fsrPixels.size() == pixels.size() &&
                    fsrPixels[fsrSample + 0] > 16 && fsrPixels[fsrSample + 0] < 239 &&
                    fsrPixels[fsrSample + 1] == fsrPixels[fsrSample + 0] &&
                    fsrPixels[fsrSample + 2] == fsrPixels[fsrSample + 0] &&
                    fsrPixels[fsrSample + 3] > 200;
                if (!fsrSampleOk) {
                    throw std::runtime_error("Vulkan FSR 1 did not preserve universal reconstructed texture sampling");
                }
                std::cout << "Vulkan capture test: " << pixels.size()
                          << " RGBA bytes, center=("
                          << static_cast<int>(pixels[center + 0]) << ","
                          << static_cast<int>(pixels[center + 1]) << ","
                          << static_cast<int>(pixels[center + 2]) << ","
                          << static_cast<int>(pixels[center + 3]) << "), high-quality sample="
                          << static_cast<int>(qualityPixels[qualitySample + 0])
                          << ", FSR 1 sample=("
                          << static_cast<int>(fsrPixels[fsrSample + 0]) << ","
                          << static_cast<int>(fsrPixels[fsrSample + 1]) << ","
                          << static_cast<int>(fsrPixels[fsrSample + 2]) << ")\n";
            }
        }

        if (normalLaunch || boundaryTest || pauseTest) {
            backend.SetUiState({});

            wb::VulkanInputTestScript testScript;
            if (boundaryTest) {
                testScript.maxFrames = 360;
                testScript.enterFrames = {2, 4};
                testScript.leftClickFrames = {12};
                testScript.overrideLogicalMouse = true;
                testScript.logicalMouse = {278.0f, 250.0f};
            } else if (pauseTest) {
                testScript.maxFrames = 120;
                testScript.enterFrames = {2, 4, 26, 40, 44};
                testScript.downFrames = {24, 38};
                testScript.rightFrames = {42};
                testScript.escapeFrames = {20, 34};
                testScript.overrideLogicalMouse = true;
                testScript.logicalMouse = {0.0f, 0.0f};
            }

            auto input = std::make_unique<wb::VulkanInputProvider>(
                hwnd,
                &g_shouldClose,
                &g_mouseWheelDelta,
                std::move(testScript)
            );
            wb::VulkanInputProvider* inputState = input.get();

            wb::GameServices services;
            services.window = std::make_unique<wb::VulkanAppWindow>(instance, hwnd, backend);
            services.audioBackend = std::make_unique<wb::MiniaudioAudioBackend>(config.assetRoot);
            services.input = std::move(input);
            services.capture = std::make_unique<wb::VulkanCaptureService>(*rendererContract);
            services.renderer = std::move(rendererContract);
            services.framePipeline = std::move(framePipeline);

            wb::SceneId finalScene = wb::SceneId::Preloader;
            std::uint64_t sharedFrames = 0;
            bool sharedExitRequested = false;
            bool pauseMenuOpened = false;
            bool pauseSettingsOpened = false;
            {
                wb::Game game(config.assetRoot, executableDirectory(), std::move(services));
                std::cout << "Vulkan shared Game started with Vulkan AppWindow/input/capture services and miniaudio playback.\n";
                game.run();
                finalScene = game.currentSceneId();
                sharedFrames = inputState->frameCount();
                sharedExitRequested = game.exitRequested();
                pauseMenuOpened = game.pauseMenuOpenedEver();
                pauseSettingsOpened = game.pauseSettingsOpenedEver();
            }

            if (boundaryTest && finalScene != wb::SceneId::Gameplay) {
                throw std::runtime_error("Vulkan shared-game test did not reach GameplayScene");
            }
            if (boundaryTest) {
                std::cout << "Vulkan shared-game test reached Game, SceneManager, GameplayScene, and the real photo capture path through VulkanRenderer.\n";
            }
            if (pauseTest && (finalScene != wb::SceneId::Gameplay || !pauseMenuOpened || !pauseSettingsOpened || !sharedExitRequested)) {
                throw std::runtime_error(
                    "Vulkan pause test did not complete pause, settings, and confirmed exit flow "
                    "(scene=" + std::to_string(static_cast<int>(finalScene)) +
                    ", pause=" + (pauseMenuOpened ? std::string("yes") : std::string("no")) +
                    ", settings=" + (pauseSettingsOpened ? std::string("yes") : std::string("no")) +
                    ", exit=" + (sharedExitRequested ? std::string("yes") : std::string("no")) +
                    ", frames=" + std::to_string(sharedFrames) + ")"
                );
            }
            if (pauseTest) {
                std::cout << "Vulkan pause test opened the gameplay pause menu, entered settings, returned, and confirmed Exit Game.\n";
            }

            backend.Shutdown();
            if (IsWindow(hwnd)) {
                DestroyWindow(hwnd);
            }
            std::cout << "Rendered shared-game frames: " << (sharedFrames > 0 ? sharedFrames - 1 : 0) << '\n';
            return sharedFrames > 1 ? 0 : 1;
        }

        const wbwwb::vulkan::VulkanBackendStats stats = backend.Stats();
        if (resolutionTest &&
            (stats.renderTargetExtent.width != 3840u || stats.renderTargetExtent.height != 2160u ||
             stats.swapchainExtent.width != 960u || stats.swapchainExtent.height != 540u)) {
            throw std::runtime_error("Vulkan selected-resolution test did not keep a 3840x2160 target independent of the 960x540 swapchain");
        }
        if (hdrTest && stats.hdrSwapchainAvailable && !stats.hdrActive) {
            throw std::runtime_error("Vulkan HDR test found an HDR surface format but did not activate it");
        }
        if (hdrAutoTest && stats.hdrActive != (stats.hdrSwapchainAvailable && stats.hdrSystemEnabled)) {
            throw std::runtime_error("Vulkan Auto HDR did not follow the active Windows display state");
        }
        if ((hdrTest || hdrAutoTest) && std::abs(stats.hdrPeakNits - 1400.0f) > 0.5f) {
            throw std::runtime_error("Vulkan HDR peak calibration did not reach the output pipeline");
        }
        if (hdrTest && std::abs(stats.hdrPaperWhiteNits - 250.0f) > 0.5f) {
            throw std::runtime_error("Vulkan HDR paper-white calibration did not reach forced HDR output");
        }
        std::cout << "Renderer backend: " << wb::configuredRenderBackendName() << '\n';
        if (smokeGameRender) {
            std::cout << "Smoke game texture: " << config.smokeTexturePath.u8string() << '\n';
        }
        if (menuTest) {
            std::cout << "Vulkan UI test: menu/preloader\n";
        }
        if (settingsTest) {
            std::cout << "Vulkan UI test: settings shell\n";
        }
        if (boundaryTest) {
            std::cout << "Vulkan UI test: shared-game boundary\n";
            std::cout << "Boundary test enters shared Game, SceneManager, GameplayScene, and Vulkan photo capture.\n";
        }
        if (pauseTest) {
            std::cout << "Vulkan UI test: gameplay pause/settings/exit flow\n";
        }
        if (rendererContractTest) {
            std::cout << "Vulkan renderer contract test: render::Renderer adapter\n";
        }
        if (captureTest) {
            std::cout << "Vulkan capture test: offscreen render target + RGBA readback\n";
        }
        if (hdrTest) {
            std::cout << "Vulkan HDR test: native HDR swapchain + color-managed present pass"
                      << (stats.hdrSwapchainAvailable ? "\n" : " (skipped: output unavailable)\n");
        }
        if (hdrAutoTest) {
            std::cout << "Vulkan Auto HDR test: follows the active Windows display state\n";
        }
        if (msaaTest) {
            const std::uint32_t activeSamples = backend.ActiveMsaaSamples();
            if (activeSamples < 2 || activeSamples > 16) {
                throw std::runtime_error("Vulkan MSAA test could not activate a multisample logical target");
            }
            std::cout << "Vulkan MSAA test: requested 16x, active " << activeSamples
                      << "x with a resolved logical target\n";
        }
        if (resolutionTest) {
            std::cout << "Vulkan selected-resolution test: 3840x2160 scene, post-process, capture, and picture resources with an independent 960x540 swapchain\n";
        }
        if (normalLaunch) {
            std::cout << "Vulkan launch mode: menu/settings scene loop\n";
            std::cout << "Settings file: " << settings.path().u8string() << '\n';
        }
        std::cout << "Swapchain: " << stats.swapchainExtent.width << "x" << stats.swapchainExtent.height
                  << ", images=" << stats.swapchainImageCount
                  << ", format=" << stats.swapchainFormat
                  << ", colorSpace=" << stats.swapchainColorSpace
                  << ", presentMode=" << stats.presentMode << '\n';
        std::cout << "Render target: " << stats.renderTargetExtent.width << "x" << stats.renderTargetExtent.height << '\n';
        std::cout << "HDR: " << (stats.hdrActive ? "active" : "inactive")
                  << " (available=" << (stats.hdrSwapchainAvailable ? "yes" : "no")
                  << ", system=" << (stats.hdrSystemEnabled ? "enabled" : "disabled")
                  << ", metadata=" << (stats.hdrMetadataAvailable ? "yes" : "no")
                  << ", paper-white=" << static_cast<int>(std::lround(stats.hdrPaperWhiteNits)) << " nits"
                  << ", peak=" << static_cast<int>(std::lround(stats.hdrPeakNits)) << " nits)\n";

        int frames = 0;
        MSG message{};
        InputTracker inputTracker;
        VulkanScene scene = VulkanScene::Menu;
        VulkanMenuSceneState menuState;
        VulkanSettingsSceneState settingsState;
        bool loggedSharedGameBoundary = false;

        if (settingsTest) {
            VulkanSettingsSceneState testSettings;
            testSettings.section = 1;
            testSettings.selected = 1;
            backend.SetUiState(makeVulkanUiState(VulkanScene::Settings, menuState, testSettings, settings, {}));
        } else if (boundaryTest) {
            backend.SetUiState(makeVulkanUiState(VulkanScene::SharedGameBoundary, menuState, settingsState, settings, {}));
        } else if (normalLaunch || menuTest) {
            backend.SetUiState(makeVulkanUiState(VulkanScene::Menu, menuState, settingsState, settings, {}));
        }

        while (!g_shouldClose) {
            const auto frameStart = std::chrono::steady_clock::now();
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    g_shouldClose = true;
                    break;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (g_shouldClose) {
                break;
            }

            if (normalLaunch) {
                const LogicalInput input = inputTracker.Mouse(hwnd, backend.FinalPresentationRect());
                const KeyEdges keys = inputTracker.Keys();

                if (scene == VulkanScene::Menu) {
                    const bool playHovered = input.inside && pointIn(input.x, input.y, 80.5f, 207.5f, 395.0f, 85.0f);
                    const bool settingsHovered = input.inside && pointIn(input.x, input.y, 716.0f, 394.0f, 154.0f, 58.0f);
                    if (playHovered) {
                        menuState.selected = 0;
                    } else if (settingsHovered) {
                        menuState.selected = 1;
                    }
                    if (keys.up || keys.down || keys.left || keys.right) {
                        menuState.selected = 1 - menuState.selected;
                    }

                    const bool playActive = playHovered || menuState.selected == 0;
                    const bool settingsActive = settingsHovered || menuState.selected == 1;
                    menuState.playHoverScale += ((playActive ? 1.04f : 1.0f) - menuState.playHoverScale) * 0.20f;
                    menuState.settingsHoverScale += ((settingsActive ? 1.08f : 1.0f) - menuState.settingsHoverScale) * 0.20f;

                    const bool activatePlay = (playHovered && input.leftPressed) ||
                        ((keys.enter || keys.space) && menuState.selected == 0);
                    const bool activateSettings = (settingsHovered && input.leftPressed) ||
                        ((keys.enter || keys.space) && menuState.selected == 1);
                    if (activatePlay) {
                        scene = VulkanScene::SharedGameBoundary;
                        if (!loggedSharedGameBoundary) {
                            std::cout << "Legacy Vulkan UI harness selected the shared-game status view.\n";
                            std::cout << "Normal launches now start the shared Game directly; --vulkan-boundary-test verifies real GameplayScene and photo capture.\n";
                            loggedSharedGameBoundary = true;
                        }
                    } else if (activateSettings) {
                        scene = VulkanScene::Settings;
                        settingsState.selected = std::clamp(settingsState.selected, 0, 1);
                        settingsState.scrollOffset = 0.0f;
                    }
                } else if (scene == VulkanScene::SharedGameBoundary) {
                    if (keys.escape || keys.backspace || keys.enter || keys.space || input.leftPressed) {
                        scene = VulkanScene::Menu;
                    }
                } else if (scene == VulkanScene::Settings) {
                    std::vector<VulkanSettingsRowDef> rows = buildSettingsRows(settings, settingsState.section);
                    clampSettingsSelection(settingsState, rows);

                    const float screenX = settingsScreenX();
                    const float screenY = settingsScreenY();
                    const float screenW = settingsScreenW();
                    if (input.inside && pointIn(input.x, input.y, screenX, screenY, screenW, settingsScreenH())) {
                        settingsState.scrollOffset = std::clamp(settingsState.scrollOffset - input.wheel * settingsRowPitch() * 2.0f,
                                                                0.0f,
                                                                maxSettingsScroll(rows.size()));
                    }

                    constexpr int tabCount = 4;
                    const float tabWidth = (screenW - 32.0f) / static_cast<float>(tabCount);
                    for (int i = 0; i < tabCount; ++i) {
                        const float tabX = screenX + 16.0f + static_cast<float>(i) * tabWidth;
                        if (input.leftPressed && pointIn(input.x, input.y, tabX, screenY + 31.0f, tabWidth - 3.0f, 12.0f)) {
                            settingsState.section = i;
                            settingsState.selected = 0;
                            settingsState.dragging = -1;
                            settingsState.scrollOffset = 0.0f;
                            rows = buildSettingsRows(settings, settingsState.section);
                        }
                    }

                    const int hovered = hoveredSettingsRow(input, settingsState, rows);
                    if (hovered >= 0) {
                        settingsState.selected = hovered;
                    }
                    if (keys.down) {
                        ++settingsState.selected;
                        clampSettingsSelection(settingsState, rows);
                        ensureSettingsSelectedVisible(settingsState, rows);
                    }
                    if (keys.up) {
                        --settingsState.selected;
                        clampSettingsSelection(settingsState, rows);
                        ensureSettingsSelectedVisible(settingsState, rows);
                    }
                    if (keys.left) {
                        adjustSettingsRow(hwnd, backend, settings, settingsState, rows, -1);
                    }
                    if (keys.right) {
                        adjustSettingsRow(hwnd, backend, settings, settingsState, rows, 1);
                    }

                    if (input.leftPressed && hovered >= 0) {
                        const VulkanSettingsRowDef& row = rows[static_cast<std::size_t>(hovered)];
                        if (row.slider && row.enabled) {
                            settingsState.dragging = hovered;
                            setSettingSlider(settings, row.kind, input.x);
                            settings.save();
                        } else if (activateSettingsRow(hwnd, backend, settings, settingsState, rows, hovered)) {
                            scene = VulkanScene::Menu;
                        }
                    } else if (keys.enter || keys.space) {
                        if (activateSettingsRow(hwnd, backend, settings, settingsState, rows, settingsState.selected)) {
                            scene = VulkanScene::Menu;
                        }
                    }

                    if (settingsState.dragging >= 0) {
                        if (input.leftDown && settingsState.dragging < static_cast<int>(rows.size())) {
                            const VulkanSettingsRowDef& row = rows[static_cast<std::size_t>(settingsState.dragging)];
                            setSettingSlider(settings, row.kind, input.x);
                        } else {
                            settingsState.dragging = -1;
                            settings.save();
                        }
                    }

                    if (keys.escape || keys.backspace) {
                        settings.save();
                        scene = VulkanScene::Menu;
                        settingsState.dragging = -1;
                    }
                }

                backend.SetUiState(makeVulkanUiState(scene, menuState, settingsState, settings, input));
            }

            const bool drawPipelineBoundary = boundaryTest || (normalLaunch && scene == VulkanScene::SharedGameBoundary);
            if ((rendererContractTest || drawPipelineBoundary || captureTest || msaaTest || resolutionTest) && rendererContract && framePipeline) {
                framePipeline->BeginLogicalDraw(settings);
                if (resolutionTest) {
                    rendererContract->DrawRectangle({0.0f, 0.0f, 960.0f, 540.0f}, {28, 28, 32, 255});
                    rendererContract->DrawRectangle({120.0f, 90.0f, 720.0f, 360.0f}, {214, 32, 32, 255});
                    rendererContract->DrawRectangleOutline({120.0f, 90.0f, 720.0f, 360.0f}, 3.0f, {255, 255, 255, 255});
                } else if (drawPipelineBoundary && !rendererContractTest) {
                    drawPipelineBoundaryScene(*rendererContract, contractFont);
                } else if (captureTest) {
                    rendererContract->DrawRectangle({0.0f, 0.0f, 960.0f, 540.0f}, {28, 28, 32, 255});
                    rendererContract->DrawRectangle({286.0f, 156.0f, 388.0f, 228.0f}, {72, 72, 72, 255});
                    rendererContract->DrawTextureSource({
                        captureTexture,
                        {0.0f, 0.0f, 128.0f, 64.0f},
                        {320.0f, 190.0f, 320.0f, 160.0f},
                        {0.0f, 0.0f},
                        0.0f,
                        {255, 255, 255, 255},
                        1.0f,
                        false,
                        false
                    });
                } else {
                    rendererContract->DrawRectangle({72.0f, 64.0f, 816.0f, 412.0f}, {18, 18, 20, 238});
                    rendererContract->DrawRectangle({72.0f, 64.0f, 816.0f, 56.0f}, {214, 32, 32, 238});
                    rendererContract->DrawRectangleOutline({72.0f, 64.0f, 816.0f, 412.0f}, 4.0f, {232, 232, 232, 255});
                    rendererContract->DrawLine({120.0f, 190.0f}, {840.0f, 190.0f}, 3.0f, {180, 180, 180, 220});
                    rendererContract->DrawCircle({190.0f + static_cast<float>(frames % 60) * 2.0f, 300.0f},
                                                 42.0f,
                                                 {240, 240, 240, 230});
                    rendererContract->DrawTexture({
                        contractTexture,
                        {0.0f, 0.0f, 2.0f, 2.0f},
                        {520.0f, 252.0f, 144.0f, 144.0f},
                        {72.0f, 72.0f},
                        0.01f * static_cast<float>(frames),
                        {255, 255, 255, 255},
                        1.0f,
                        false,
                        false
                    });
                    rendererContract->DrawTextureQuad({
                        contractTexture,
                        {0.0f, 0.0f, 2.0f, 2.0f},
                        {{
                            {686.0f, 244.0f},
                            {662.0f, 382.0f},
                            {806.0f, 358.0f},
                            {832.0f, 228.0f}
                        }},
                        {255, 255, 255, 220},
                        0.92f,
                        static_cast<bool>((frames / 45) % 2),
                        false
                    });
                    rendererContract->PushScissor({110.0f, 420.0f, 740.0f, 28.0f});
                    rendererContract->DrawRectangle({90.0f, 414.0f, 780.0f, 40.0f}, {214, 32, 32, 220});
                    rendererContract->PopScissor();
                    rendererContract->DrawText({
                        contractFont,
                        "Vulkan render::Renderer path",
                        {112.0f, 78.0f},
                        34.0f,
                        {255, 255, 255, 255},
                        1.0f
                    });
                }
                framePipeline->EndLogicalDraw(settings, nullptr, nullptr, 0.0f);
            } else {
                if (!backend.BeginFrame()) {
                    std::cerr << "Vulkan swapchain became out of date before rendering a frame.\n";
                    break;
                }
                backend.ClearFrame();
                backend.EndFrame();
                if (!backend.PresentFrame()) {
                    std::cerr << "Vulkan swapchain became out of date during present.\n";
                    break;
                }
            }

            ++frames;
            if (resolutionTest && frames == 3 && rendererContract) {
                rendererContract->SetRenderResolution(2560u, 1440u);
                resolutionAssets->updateRenderResolution(2560u, 1440u);
                const wb::render::RendererStats resized = rendererContract->Stats();
                const wb::render::TextureHandle resizedPicture = resolutionAssets->textureHandle("sprites/bg_preload.png");
                const wb::render::Vec2 resizedPictureSize = rendererContract->TextureSize(resizedPicture);
                if (resized.renderWidth != 2560u || resized.renderHeight != 1440u ||
                    static_cast<std::uint32_t>(std::lround(resizedPictureSize.x)) != 2560u ||
                    static_cast<std::uint32_t>(std::lround(resizedPictureSize.y)) != 1440u) {
                    throw std::runtime_error("Vulkan live render-resolution change did not create a 2560x1440 target");
                }
            }
              if (resolutionTest && frames == 5 && rendererContract) {
                  rendererContract->SetRenderResolution(3840u, 2160u);
                  resolutionAssets->updateRenderResolution(3840u, 2160u);
                const wb::render::RendererStats restored = rendererContract->Stats();
                const wb::render::TextureHandle restoredPicture = resolutionAssets->textureHandle("sprites/bg_preload.png");
                const wb::render::Vec2 restoredPictureSize = rendererContract->TextureSize(restoredPicture);
                if (restored.renderWidth != 3840u || restored.renderHeight != 2160u ||
                    static_cast<std::uint32_t>(std::lround(restoredPictureSize.x)) != 3840u ||
                    static_cast<std::uint32_t>(std::lround(restoredPictureSize.y)) != 2160u) {
                      throw std::runtime_error("Vulkan live render-resolution change did not restore the 3840x2160 target");
                  }
              }
              if (resolutionTest && frames == 6 && rendererContract) {
                  const wb::render::TextureHandle previousPicture = resolutionAssets->textureHandle("sprites/bg_preload.png");
                  resolutionAssets->setSmoothTextures(false);
                  resolutionAssets->updateRenderResolution(3840u, 2160u);
                  const wb::render::TextureHandle nearestPicture = resolutionAssets->textureHandle("sprites/bg_preload.png");
                  const wb::render::Vec2 nearestPictureSize = rendererContract->TextureSize(nearestPicture);
                  if (nearestPicture.id == previousPicture.id ||
                      static_cast<std::uint32_t>(std::lround(nearestPictureSize.x)) != 3840u ||
                      static_cast<std::uint32_t>(std::lround(nearestPictureSize.y)) != 2160u) {
                      throw std::runtime_error("Vulkan picture filtering change did not reload the 3840x2160 resource");
                  }
              }
            if (automatedTest && frames == 60) {
                if ((rendererContractTest || boundaryTest || captureTest || msaaTest) && rendererContract) {
                    rendererContract->Resize(static_cast<std::uint32_t>(kSmokeWidth),
                                             static_cast<std::uint32_t>(kSmokeHeight));
                } else {
                    backend.Resize(static_cast<std::uint32_t>(kSmokeWidth), static_cast<std::uint32_t>(kSmokeHeight));
                }
            }
            if (resolutionTest && frames >= 8) {
                break;
            }
            if (automatedTest && frames >= 120) {
                break;
            }

            const int targetFps = normalLaunch ? frameLimitFps(settings.frameLimit) : 0;
            if (targetFps > 0) {
                const auto targetFrameDuration = std::chrono::microseconds(1000000 / targetFps);
                const auto elapsed = std::chrono::steady_clock::now() - frameStart;
                if (elapsed < targetFrameDuration) {
                    std::this_thread::sleep_for(targetFrameDuration - elapsed);
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        if (resolutionAssets) {
            resolutionAssets->releaseTexture("sprites/bg_preload.png");
            resolutionAssets->setRenderer(nullptr);
            resolutionAssets.reset();
        }
        framePipeline.reset();
        if (rendererContract) {
            rendererContract->Shutdown();
            rendererContract.reset();
        }
        backend.Shutdown();
        if (IsWindow(hwnd)) {
            DestroyWindow(hwnd);
        }
        std::cout << "Rendered frames: " << frames << '\n';
        return frames > 0 ? 0 : 1;
    } catch (const std::exception& ex) {
        std::cerr << "Vulkan backend smoke failed: " << ex.what() << '\n';
        if (!automatedTest) {
            MessageBoxA(nullptr, ex.what(), "Native WBWWB C++ Port - Vulkan Smoke Test Failed", MB_OK | MB_ICONERROR);
        }
        return 1;
    }
}
