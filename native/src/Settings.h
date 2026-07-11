#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace wb {

struct WindowSizeOption {
    int width;
    int height;
    std::string label;
};

std::vector<WindowSizeOption> detectSystemWindowSizeOptions();

class GameSettings {
public:
    GameSettings();

    void load();
    void save() const;
    void resetDefaults();
    void refreshWindowSizeOptions(const std::vector<WindowSizeOption>& systemOptions);
    int windowSizeOptionCount() const { return static_cast<int>(windowSizeOptions_.size()); }
    const WindowSizeOption& windowSizeOption(int index) const;
    const WindowSizeOption& currentWindowSize() const;
    void setWindowSizeIndex(int index);
    void stepWindowSize(int direction);
    void syncWindowSizeToScale(int monitorWidth, int monitorHeight);
    void stepDisplayMode(int direction);
    void stepMonitor(int direction, int monitorCount);
    void clampMonitorIndex(int monitorCount);
    void stepWindowScale(int direction);
    void stepFrameLimit(int direction);
    void stepMsaaSamples(int direction);
    void stepHdrMode(int direction);

    const std::filesystem::path& path() const { return path_; }

    float masterVolume = 1.0f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
    bool fullscreen = false;
    int displayMode = 0;
    int monitorIndex = 0;
    int windowScale = 4;
    bool integerScaling = true;
    int aspectFit = 0;
    bool vsync = true;
    int outputSmoothing = 1;
    bool fxaa = false;
    int photoUpscaler = 1;
    int msaaSamples = 0;
    int crtFilter = 0;
    int screenNoise = 0;
    float sharpness = 1.0f;
    float gamma = 1.0f;
    float brightness = 1.0f;
    float contrast = 1.0f;
    float blackLevel = 0.0f;
    float whiteLevel = 1.0f;
    float safeArea = 1.0f;
    int frameLimit = 1;
    bool fpsCounter = false;
    int screenBorder = 0;
    int scanlines = 0;
    int crtCurve = 0;
    bool reduceFlashing = false;
    int hdrMode = 0;
    float hdrPaperWhiteNits = 203.0f;
    float hdrPeakNits = 1000.0f;
    float hdrHighlightStrength = 0.35f;
    std::string languageId = "en";
    int windowSizeIndex = 0;
    int windowWidth = 960;
    int windowHeight = 540;

private:
    void clampValues();
    void syncWindowSizeIndexToSize();

    std::vector<WindowSizeOption> windowSizeOptions_;
    std::filesystem::path path_ = std::filesystem::current_path() / "wbwwb_settings.json";
};

}  // namespace wb
