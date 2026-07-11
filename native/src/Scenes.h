#pragma once

#include "Constants.h"
#include "Scene.h"
#include "Tween.h"
#include "compat/RaylibCompat.h"

#include <string>

namespace wb {

class PreloaderScene final : public Scene {
public:
    explicit PreloaderScene(Game& game);
    void enter() override;
    void exit() override;
    void update(float dt) override;
    void draw() override;

private:
    float playHoverScale_ = 1.0f;
    float settingsHoverScale_ = 1.0f;
    int selected_ = 0;
};

class SettingsScene final : public Scene {
public:
    enum class ReturnTarget {
        MainMenu,
        PauseMenu
    };

    explicit SettingsScene(Game& game, ReturnTarget returnTarget = ReturnTarget::MainMenu);
    void enter() override;
    void exit() override;
    void update(float dt) override;
    void draw() override;

private:
    enum class Section {
        Audio,
        Display,
        Visuals,
        Monitor,
        Count
    };

    enum class Row {
        Section,
        MasterVolume,
        MusicVolume,
        SfxVolume,
        Language,
        MsaaSamples,
        Fxaa,
        PhotoUpscaler,
        CrtFilter,
        ScreenNoise,
        DisplayMode,
        Monitor,
        WindowScale,
        WindowSize,
        IntegerScaling,
        AspectFit,
        Vsync,
        OutputSmoothing,
        Sharpness,
        Gamma,
        Brightness,
        Contrast,
        BlackLevel,
        WhiteLevel,
        SafeArea,
        FrameLimit,
        FpsCounter,
        ScreenBorder,
        Scanlines,
        CrtCurve,
        ReduceFlashing,
        HdrOutput,
        HdrPaperWhite,
        HdrPeakBrightness,
        HdrHighlights,
        Reset,
        Back
    };

    Rectangle tvScreenRect() const;
    Rectangle rowsClipRect() const;
    Rectangle rowRect(int index) const;
    Rectangle sliderRect(int index) const;
    float rowPitch() const;
    float maxScrollOffset() const;
    void clampScroll();
    void ensureSelectedVisible();
    int rowCount() const;
    Row rowAt(int index) const;
    void stepSection(int direction);
    void activateRow(int index);
    void adjustRow(int index, int direction);
    void setSliderFromMouse(int index, float x);
    void saveAndApply(bool resizeWindow = true);
    void goBack();
    std::string rowLabel(int index) const;
    std::string rowValue(int index) const;
    bool isSlider(int index) const;
    float sliderValue(int index) const;
    void setSliderValue(int index, float value);

    Section section_ = Section::Audio;
    int selected_ = 0;
    int dragging_ = -1;
    float scrollOffset_ = 0.0f;
    ReturnTarget returnTarget_ = ReturnTarget::MainMenu;
};

class QuoteScene final : public Scene {
public:
    explicit QuoteScene(Game& game);
    void enter() override;
    void exit() override;
    void update(float dt) override;
    void draw() override;

private:
    float elapsed_ = 0.0f;
    bool ambienceStarted_ = false;
};

class GameplayScene final : public Scene {
public:
    explicit GameplayScene(Game& game);
    ~GameplayScene() override;
    void enter() override;
    void exit() override;
    void update(float dt) override;
    void draw() override;

private:
    class Impl;
    Impl* impl_ = nullptr;
};

class CreditsScene final : public Scene {
public:
    explicit CreditsScene(Game& game);
    void enter() override;
    void exit() override;
    void update(float dt) override;
    void draw() override;

private:
    float elapsed_ = 0.0f;
};

class PostCreditsScene final : public Scene {
public:
    explicit PostCreditsScene(Game& game);
    void enter() override;
    void update(float dt) override;
    void draw() override;

private:
    void drawWorld(Vector2 screenOrigin, Vector2 worldTopLeft, float scale, bool includeStream);
    void drawCamera();

    float elapsed_ = 0.0f;
    float fade_ = 1.0f;
    Vector2 camera_{static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)};
};

class FinalScene final : public Scene {
public:
    explicit FinalScene(Game& game);
    void enter() override;
    void update(float dt) override;
    void draw() override;
};

}  // namespace wb
