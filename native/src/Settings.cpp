#include "Settings.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace wb {
namespace {

constexpr std::array<std::pair<int, int>, 6> kCommonWindowSizes{{
    {960, 540},
    {1280, 720},
    {1600, 900},
    {1920, 1080},
    {2560, 1440},
    {3840, 2160}
}};

constexpr std::array<int, 5> kMsaaSampleOptions{{0, 2, 4, 8, 16}};
constexpr std::array<int, 4> kFrameLimitOptions{{30, 60, 120, 0}};
constexpr int kRenderQualityVersion = 2;

std::string labelForSize(int width, int height) {
    std::string label = std::to_string(width) + " x " + std::to_string(height);
    if (width == 3840 && height == 2160) {
        label += " (4K)";
    }
    return label;
}

bool isValidWindowSize(int width, int height) {
    return width >= 640 && height >= 360 && width <= 7680 && height <= 4320;
}

int namedLevel(const nlohmann::json& json, const char* indexKey, const char* labelKey, int current, std::initializer_list<const char*> labels) {
    if (json.contains(indexKey)) {
        return json.value(indexKey, current);
    }

    if (!json.contains(labelKey)) {
        return current;
    }

    const nlohmann::json& value = json.at(labelKey);
    if (value.is_number_integer()) {
        return value.get<int>();
    }
    if (!value.is_string()) {
        return current;
    }

    const std::string label = value.get<std::string>();
    int index = 0;
    for (const char* candidate : labels) {
        if (label == candidate) {
            return index;
        }
        ++index;
    }
    return current;
}

const char* outputSmoothingLabel(int value) {
    return value == 1 ? "Linear" : "Nearest";
}

const char* photoUpscalerLabel(int value) {
    return value == 1 ? "AMD FSR 1" : "Off";
}

const char* displayModeLabel(int value) {
    switch (value) {
        case 1: return "Borderless";
        case 2: return "Fullscreen";
        default: return "Windowed";
    }
}

const char* windowScaleLabel(int value) {
    switch (value) {
        case 0: return "1x";
        case 1: return "2x";
        case 2: return "3x";
        case 3: return "4x";
        case 5: return "Custom";
        default: return "Fit";
    }
}

const char* aspectFitLabel(int value) {
    return value == 1 ? "Overscan" : "Letterbox";
}

const char* frameLimitLabel(int value) {
    switch (value) {
        case 0: return "30 FPS";
        case 2: return "120 FPS";
        case 3: return "Unlimited";
        default: return "60 FPS";
    }
}

int normalizeMsaaSamples(int value) {
    if (value <= 0) {
        return 0;
    }
    int best = 2;
    int bestDistance = std::abs(value - best);
    for (int option : kMsaaSampleOptions) {
        const int distance = std::abs(value - option);
        if (distance < bestDistance) {
            best = option;
            bestDistance = distance;
        }
    }
    return best;
}

int msaaSamplesFromLabel(const std::string& label, int current) {
    if (label == "Off") return 0;
    if (label == "2x") return 2;
    if (label == "4x") return 4;
    if (label == "8x") return 8;
    if (label == "16x") return 16;
    return current;
}

int readMsaaSamples(const nlohmann::json& json, int current) {
    if (json.contains("msaaSamples")) {
        return normalizeMsaaSamples(json.value("msaaSamples", current));
    }
    if (json.contains("msaa")) {
        const nlohmann::json& value = json.at("msaa");
        if (value.is_number_integer()) {
            return normalizeMsaaSamples(value.get<int>());
        }
        if (value.is_string()) {
            return msaaSamplesFromLabel(value.get<std::string>(), current);
        }
    }
    if (json.contains("msaa4x")) {
        return json.value("msaa4x", false) ? 4 : 0;
    }
    return current;
}

const char* msaaSamplesLabel(int value) {
    switch (normalizeMsaaSamples(value)) {
        case 2: return "2x";
        case 4: return "4x";
        case 8: return "8x";
        case 16: return "16x";
        default: return "Off";
    }
}

const char* crtFilterLabel(int value) {
    switch (value) {
        case 1: return "Subtle";
        case 2: return "Strong";
        default: return "Off";
    }
}

const char* screenNoiseLabel(int value) {
    switch (value) {
        case 1: return "Low";
        case 2: return "Medium";
        case 3: return "High";
        default: return "Off";
    }
}

const char* screenBorderLabel(int value) {
    switch (value) {
        case 1: return "Subtle";
        case 2: return "Strong";
        default: return "Off";
    }
}

const char* scanlinesLabel(int value) {
    switch (value) {
        case 1: return "Light";
        case 2: return "Medium";
        case 3: return "Heavy";
        default: return "Off";
    }
}

const char* crtCurveLabel(int value) {
    switch (value) {
        case 1: return "Light";
        case 2: return "Medium";
        default: return "Off";
    }
}

void addWindowSizeOption(std::vector<WindowSizeOption>& options, int width, int height, const std::string& label = {}) {
    if (!isValidWindowSize(width, height)) {
        return;
    }
    const auto sameSize = [width, height](const WindowSizeOption& option) {
        return option.width == width && option.height == height;
    };
    if (std::any_of(options.begin(), options.end(), sameSize)) {
        return;
    }
    options.push_back({width, height, label.empty() ? labelForSize(width, height) : label});
}

void sortWindowSizeOptions(std::vector<WindowSizeOption>& options) {
    std::sort(options.begin(), options.end(), [](const WindowSizeOption& a, const WindowSizeOption& b) {
        const long long aPixels = static_cast<long long>(a.width) * static_cast<long long>(a.height);
        const long long bPixels = static_cast<long long>(b.width) * static_cast<long long>(b.height);
        if (aPixels != bPixels) {
            return aPixels < bPixels;
        }
        if (a.width != b.width) {
            return a.width < b.width;
        }
        return a.height < b.height;
    });
}

}  // namespace

std::vector<WindowSizeOption> detectSystemWindowSizeOptions() {
    std::vector<WindowSizeOption> options;

#ifdef _WIN32
    DISPLAY_DEVICEW device{};
    device.cb = sizeof(device);
    for (DWORD deviceIndex = 0; EnumDisplayDevicesW(nullptr, deviceIndex, &device, 0); ++deviceIndex) {
        if ((device.StateFlags & DISPLAY_DEVICE_ACTIVE) == 0) {
            device = {};
            device.cb = sizeof(device);
            continue;
        }

        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        for (DWORD modeIndex = 0; EnumDisplaySettingsW(device.DeviceName, modeIndex, &mode); ++modeIndex) {
            addWindowSizeOption(options, static_cast<int>(mode.dmPelsWidth), static_cast<int>(mode.dmPelsHeight));
            mode = {};
            mode.dmSize = sizeof(mode);
        }

        device = {};
        device.cb = sizeof(device);
    }
#endif

    sortWindowSizeOptions(options);
    return options;
}

GameSettings::GameSettings() {
    refreshWindowSizeOptions({});
}

void GameSettings::load() {
    resetDefaults();

    std::ifstream input(path_);
    if (!input) {
        return;
    }

    try {
        bool saveAfterLoad = false;
        nlohmann::json json;
        input >> json;
        masterVolume = json.value("masterVolume", masterVolume);
        musicVolume = json.value("musicVolume", musicVolume);
        sfxVolume = json.value("sfxVolume", sfxVolume);
        fullscreen = json.value("fullscreen", fullscreen);
        displayMode = fullscreen ? 2 : 0;
        displayMode = namedLevel(json, "displayModeIndex", "displayMode", displayMode, {"Windowed", "Borderless", "Fullscreen"});
        displayMode = namedLevel(json, "display_mode_index", "display_mode", displayMode, {"Windowed", "Borderless", "Fullscreen"});
        monitorIndex = json.value("monitorIndex", monitorIndex);
        monitorIndex = json.value("monitor_index", monitorIndex);
        const bool hasWindowScale =
            json.contains("windowScaleIndex") ||
            json.contains("windowScale") ||
            json.contains("window_scale_index") ||
            json.contains("window_scale");
        const bool hasWindowSize =
            json.contains("windowSizeIndex") ||
            json.contains("windowWidth") ||
            json.contains("windowHeight") ||
            json.contains("windowSize");
        windowScale = namedLevel(json, "windowScaleIndex", "windowScale", windowScale, {"1x", "2x", "3x", "4x", "Fit", "Custom"});
        windowScale = namedLevel(json, "window_scale_index", "window_scale", windowScale, {"1x", "2x", "3x", "4x", "Fit", "Custom"});
        if (hasWindowSize && !hasWindowScale) {
            windowScale = 5;
        }
        integerScaling = json.value("integerScaling", integerScaling);
        integerScaling = json.value("integer_scaling", integerScaling);
        aspectFit = namedLevel(json, "aspectFitIndex", "aspectFit", aspectFit, {"Letterbox", "Overscan"});
        aspectFit = namedLevel(json, "aspect_fit_index", "aspect_fit", aspectFit, {"Letterbox", "Overscan"});
        vsync = json.value("vsync", vsync);
        outputSmoothing = namedLevel(json, "outputSmoothingIndex", "outputSmoothing", outputSmoothing, {"Nearest", "Linear"});
        outputSmoothing = namedLevel(json, "output_filter_index", "output_filter", outputSmoothing, {"Nearest", "Linear"});
        if (!json.contains("outputSmoothingIndex") &&
            !json.contains("outputSmoothing") &&
            !json.contains("output_filter_index") &&
            !json.contains("output_filter") &&
            json.contains("antiAliasing")) {
            outputSmoothing = json.value("antiAliasing", false) ? 1 : 0;
        }
        const int renderQualityVersion = json.value("render_quality_version", json.value("renderQualityVersion", 0));
        if (renderQualityVersion < kRenderQualityVersion) {
            outputSmoothing = 1;
            saveAfterLoad = true;
        }
        fxaa = json.value("fxaa", fxaa);
        photoUpscaler = namedLevel(json, "photoUpscalerIndex", "photoUpscaler", photoUpscaler, {"Off", "AMD FSR 1"});
        photoUpscaler = namedLevel(json, "photo_upscaler_index", "photo_upscaler", photoUpscaler, {"Off", "AMD FSR 1"});
        msaaSamples = readMsaaSamples(json, msaaSamples);
        crtFilter = namedLevel(json, "crtFilterIndex", "crtFilter", crtFilter, {"Off", "Subtle", "Strong"});
        screenNoise = namedLevel(json, "screenNoiseIndex", "screenNoise", screenNoise, {"Off", "Low", "Medium", "High"});
        sharpness = json.value("sharpness", sharpness);
        gamma = json.value("gamma", gamma);
        brightness = json.value("brightness", brightness);
        contrast = json.value("contrast", contrast);
        blackLevel = json.value("blackLevel", blackLevel);
        blackLevel = json.value("black_level", blackLevel);
        whiteLevel = json.value("whiteLevel", whiteLevel);
        whiteLevel = json.value("white_level", whiteLevel);
        safeArea = json.value("safeArea", safeArea);
        safeArea = json.value("safe_area", safeArea);
        frameLimit = namedLevel(json, "frameLimitIndex", "frameLimit", frameLimit, {"30 FPS", "60 FPS", "120 FPS", "Unlimited"});
        frameLimit = namedLevel(json, "frame_limit_index", "frame_limit", frameLimit, {"30 FPS", "60 FPS", "120 FPS", "Unlimited"});
        fpsCounter = json.value("fpsCounter", fpsCounter);
        fpsCounter = json.value("fps_counter", fpsCounter);
        screenBorder = namedLevel(json, "screenBorderIndex", "screenBorder", screenBorder, {"Off", "Subtle", "Strong"});
        screenBorder = namedLevel(json, "screen_border_index", "screen_border", screenBorder, {"Off", "Subtle", "Strong"});
        scanlines = namedLevel(json, "scanlinesIndex", "scanlines", scanlines, {"Off", "Light", "Medium", "Heavy"});
        scanlines = namedLevel(json, "scanlines_index", "scanlines", scanlines, {"Off", "Light", "Medium", "Heavy"});
        crtCurve = namedLevel(json, "crtCurveIndex", "crtCurve", crtCurve, {"Off", "Light", "Medium"});
        crtCurve = namedLevel(json, "crt_curve_index", "crt_curve", crtCurve, {"Off", "Light", "Medium"});
        reduceFlashing = json.value("reduceFlashing", reduceFlashing);
        reduceFlashing = json.value("reduce_flashing", reduceFlashing);
        hdrMode = namedLevel(json, "hdr_mode_index", "hdr_mode", hdrMode, {"Off", "Auto", "On"});
        hdrPaperWhiteNits = json.value("hdrPaperWhiteNits", hdrPaperWhiteNits);
        hdrPaperWhiteNits = json.value("hdr_paper_white_nits", hdrPaperWhiteNits);
        hdrPeakNits = json.value("hdrPeakNits", hdrPeakNits);
        hdrPeakNits = json.value("hdr_peak_nits", hdrPeakNits);
        hdrHighlightStrength = json.value("hdrHighlightStrength", hdrHighlightStrength);
        hdrHighlightStrength = json.value("hdr_highlight_strength", hdrHighlightStrength);
        languageId = json.value("language_id", json.value("languageId", languageId));
        const int savedIndex = json.value("windowSizeIndex", windowSizeIndex);
        if (json.contains("windowWidth") && json.contains("windowHeight")) {
            windowWidth = json.value("windowWidth", windowWidth);
            windowHeight = json.value("windowHeight", windowHeight);
        } else if (json.contains("windowSize")) {
            const std::string savedLabel = json.value("windowSize", std::string{});
            bool matchedLabel = false;
            for (int i = 0; i < windowSizeOptionCount(); ++i) {
                const WindowSizeOption& option = windowSizeOption(i);
                if (option.label == savedLabel) {
                    windowWidth = option.width;
                    windowHeight = option.height;
                    matchedLabel = true;
                    break;
                }
            }
            if (!matchedLabel) {
                setWindowSizeIndex(savedIndex);
            }
        } else {
            setWindowSizeIndex(savedIndex);
        }
        clampValues();
        if (saveAfterLoad) {
            save();
        }
    } catch (...) {
        resetDefaults();
    }
}

void GameSettings::save() const {
    nlohmann::json json{
        {"masterVolume", masterVolume},
        {"musicVolume", musicVolume},
        {"sfxVolume", sfxVolume},
        {"fullscreen", displayMode == 2},
        {"renderQualityVersion", kRenderQualityVersion},
        {"render_quality_version", kRenderQualityVersion},
        {"display_mode_index", displayMode},
        {"display_mode", displayModeLabel(displayMode)},
        {"monitor_index", monitorIndex},
        {"window_scale_index", windowScale},
        {"window_scale", windowScaleLabel(windowScale)},
        {"integer_scaling", integerScaling},
        {"aspect_fit_index", aspectFit},
        {"aspect_fit", aspectFitLabel(aspectFit)},
        {"vsync", vsync},
        {"outputSmoothingIndex", outputSmoothing},
        {"outputSmoothing", outputSmoothingLabel(outputSmoothing)},
        {"output_filter_index", outputSmoothing},
        {"output_filter", outputSmoothingLabel(outputSmoothing)},
        {"fxaa", fxaa},
        {"photoUpscalerIndex", photoUpscaler},
        {"photoUpscaler", photoUpscalerLabel(photoUpscaler)},
        {"photo_upscaler_index", photoUpscaler},
        {"photo_upscaler", photoUpscalerLabel(photoUpscaler)},
        {"msaaSamples", msaaSamples},
        {"msaa", msaaSamplesLabel(msaaSamples)},
        {"crtFilterIndex", crtFilter},
        {"crtFilter", crtFilterLabel(crtFilter)},
        {"screenNoiseIndex", screenNoise},
        {"screenNoise", screenNoiseLabel(screenNoise)},
        {"sharpness", sharpness},
        {"gamma", gamma},
        {"brightness", brightness},
        {"contrast", contrast},
        {"black_level", blackLevel},
        {"white_level", whiteLevel},
        {"safe_area", safeArea},
        {"frame_limit_index", frameLimit},
        {"frame_limit", frameLimitLabel(frameLimit)},
        {"fps_counter", fpsCounter},
        {"screen_border_index", screenBorder},
        {"screen_border", screenBorderLabel(screenBorder)},
        {"scanlines_index", scanlines},
        {"scanlines", scanlinesLabel(scanlines)},
        {"crt_curve_index", crtCurve},
        {"crt_curve", crtCurveLabel(crtCurve)},
        {"reduce_flashing", reduceFlashing},
        {"hdr_mode_index", hdrMode},
        {"hdr_mode", hdrMode == 2 ? "On" : hdrMode == 1 ? "Auto" : "Off"},
        {"hdrPaperWhiteNits", hdrPaperWhiteNits},
        {"hdr_paper_white_nits", hdrPaperWhiteNits},
        {"hdrPeakNits", hdrPeakNits},
        {"hdr_peak_nits", hdrPeakNits},
        {"hdrHighlightStrength", hdrHighlightStrength},
        {"hdr_highlight_strength", hdrHighlightStrength},
        {"language_id", languageId},
        {"windowSizeIndex", windowSizeIndex},
        {"windowWidth", windowWidth},
        {"windowHeight", windowHeight},
        {"windowSize", currentWindowSize().label}
    };

    std::ofstream output(path_);
    if (output) {
        output << json.dump(2) << '\n';
    }
}

void GameSettings::resetDefaults() {
    masterVolume = 1.0f;
    musicVolume = 1.0f;
    sfxVolume = 1.0f;
    fullscreen = false;
    displayMode = 0;
    monitorIndex = 0;
    windowScale = 4;
    integerScaling = true;
    aspectFit = 0;
    vsync = true;
    outputSmoothing = 1;
    fxaa = false;
    photoUpscaler = 1;
    msaaSamples = 0;
    crtFilter = 0;
    screenNoise = 0;
    sharpness = 1.0f;
    gamma = 1.0f;
    brightness = 1.0f;
    contrast = 1.0f;
    blackLevel = 0.0f;
    whiteLevel = 1.0f;
    safeArea = 1.0f;
    frameLimit = 1;
    fpsCounter = false;
    screenBorder = 0;
    scanlines = 0;
    crtCurve = 0;
    reduceFlashing = false;
    hdrMode = 0;
    hdrPaperWhiteNits = 203.0f;
    hdrPeakNits = 1000.0f;
    hdrHighlightStrength = 0.35f;
    languageId = "en";
    windowSizeIndex = 0;
    windowWidth = 960;
    windowHeight = 540;
    syncWindowSizeIndexToSize();
}

void GameSettings::refreshWindowSizeOptions(const std::vector<WindowSizeOption>& systemOptions) {
    std::vector<WindowSizeOption> options;
    for (const auto& [width, height] : kCommonWindowSizes) {
        addWindowSizeOption(options, width, height);
    }
    for (const WindowSizeOption& option : systemOptions) {
        addWindowSizeOption(options, option.width, option.height, option.label);
    }
    addWindowSizeOption(options, windowWidth, windowHeight);
    sortWindowSizeOptions(options);
    windowSizeOptions_ = std::move(options);
    syncWindowSizeIndexToSize();
}

const WindowSizeOption& GameSettings::windowSizeOption(int index) const {
    const int clamped = std::clamp(index, 0, std::max(0, windowSizeOptionCount() - 1));
    return windowSizeOptions_[static_cast<std::size_t>(clamped)];
}

const WindowSizeOption& GameSettings::currentWindowSize() const {
    return windowSizeOption(windowSizeIndex);
}

void GameSettings::setWindowSizeIndex(int index) {
    windowSizeIndex = std::clamp(index, 0, std::max(0, windowSizeOptionCount() - 1));
    const WindowSizeOption& selected = currentWindowSize();
    windowWidth = selected.width;
    windowHeight = selected.height;
}

void GameSettings::stepWindowSize(int direction) {
    const int count = windowSizeOptionCount();
    if (count <= 0) {
        return;
    }
    setWindowSizeIndex((windowSizeIndex + direction + count) % count);
    windowScale = 5;
}

void GameSettings::syncWindowSizeToScale(int monitorWidth, int monitorHeight) {
    const int safeMonitorWidth = std::clamp(monitorWidth, 640, 7680);
    const int safeMonitorHeight = std::clamp(monitorHeight, 360, 4320);
    const float fitScale = std::max(0.1f, std::min(
        static_cast<float>(safeMonitorWidth) / 960.0f,
        static_cast<float>(safeMonitorHeight) / 540.0f
    ));
    const float requestedScale = windowScale >= 4 ? fitScale : static_cast<float>(windowScale + 1);
    const float appliedScale = std::max(0.1f, std::min(requestedScale, fitScale));
    windowWidth = std::clamp(static_cast<int>(std::floor(960.0f * appliedScale)), 640, 7680);
    windowHeight = std::clamp(static_cast<int>(std::floor(540.0f * appliedScale)), 360, 4320);
    syncWindowSizeIndexToSize();
}

void GameSettings::stepDisplayMode(int direction) {
    displayMode = (displayMode + direction + 3) % 3;
    fullscreen = displayMode == 2;
}

void GameSettings::stepMonitor(int direction, int monitorCount) {
    const int count = std::max(1, monitorCount);
    monitorIndex = (monitorIndex + direction + count) % count;
}

void GameSettings::clampMonitorIndex(int monitorCount) {
    monitorIndex = std::clamp(monitorIndex, 0, std::max(0, monitorCount - 1));
}

void GameSettings::stepWindowScale(int direction) {
    windowScale = (windowScale + direction + 6) % 6;
}

void GameSettings::stepFrameLimit(int direction) {
    const int count = static_cast<int>(kFrameLimitOptions.size());
    frameLimit = (frameLimit + direction + count) % count;
}

void GameSettings::stepMsaaSamples(int direction) {
    auto it = std::find(kMsaaSampleOptions.begin(), kMsaaSampleOptions.end(), normalizeMsaaSamples(msaaSamples));
    int index = it == kMsaaSampleOptions.end() ? 0 : static_cast<int>(std::distance(kMsaaSampleOptions.begin(), it));
    const int count = static_cast<int>(kMsaaSampleOptions.size());
    index = (index + direction + count) % count;
    msaaSamples = kMsaaSampleOptions[static_cast<std::size_t>(index)];
}

void GameSettings::stepHdrMode(int direction) {
    hdrMode = (hdrMode + direction + 3) % 3;
}

void GameSettings::clampValues() {
    masterVolume = std::clamp(masterVolume, 0.0f, 1.0f);
    musicVolume = std::clamp(musicVolume, 0.0f, 1.0f);
    sfxVolume = std::clamp(sfxVolume, 0.0f, 1.0f);
    displayMode = std::clamp(displayMode, 0, 2);
    fullscreen = displayMode == 2;
    monitorIndex = std::max(0, monitorIndex);
    windowScale = std::clamp(windowScale, 0, 5);
    aspectFit = std::clamp(aspectFit, 0, 1);
    outputSmoothing = std::clamp(outputSmoothing, 0, 1);
    photoUpscaler = std::clamp(photoUpscaler, 0, 1);
    msaaSamples = normalizeMsaaSamples(msaaSamples);
    crtFilter = std::clamp(crtFilter, 0, 2);
    screenNoise = std::clamp(screenNoise, 0, 3);
    sharpness = std::clamp(sharpness, 0.0f, 2.0f);
    gamma = std::clamp(gamma, 0.8f, 1.2f);
    brightness = std::clamp(brightness, 0.5f, 1.5f);
    contrast = std::clamp(contrast, 0.5f, 1.5f);
    blackLevel = std::clamp(blackLevel, 0.0f, 0.2f);
    whiteLevel = std::clamp(whiteLevel, 0.8f, 1.2f);
    safeArea = std::clamp(safeArea, 0.9f, 1.0f);
    frameLimit = std::clamp(frameLimit, 0, 3);
    screenBorder = std::clamp(screenBorder, 0, 2);
    scanlines = std::clamp(scanlines, 0, 3);
    crtCurve = std::clamp(crtCurve, 0, 2);
    hdrMode = std::clamp(hdrMode, 0, 2);
    hdrPaperWhiteNits = std::clamp(hdrPaperWhiteNits, 80.0f, 500.0f);
    hdrPeakNits = std::clamp(hdrPeakNits, std::max(400.0f, hdrPaperWhiteNits), 4000.0f);
    hdrHighlightStrength = std::clamp(hdrHighlightStrength, 0.0f, 1.0f);
    if (languageId.empty()) {
        languageId = "en";
    }
    windowWidth = std::clamp(windowWidth, 640, 7680);
    windowHeight = std::clamp(windowHeight, 360, 4320);
    syncWindowSizeIndexToSize();
}

void GameSettings::syncWindowSizeIndexToSize() {
    addWindowSizeOption(windowSizeOptions_, windowWidth, windowHeight);
    sortWindowSizeOptions(windowSizeOptions_);
    for (int i = 0; i < windowSizeOptionCount(); ++i) {
        const WindowSizeOption& option = windowSizeOptions_[static_cast<std::size_t>(i)];
        if (option.width == windowWidth && option.height == windowHeight) {
            windowSizeIndex = i;
            return;
        }
    }
    windowSizeIndex = 0;
}

}  // namespace wb
