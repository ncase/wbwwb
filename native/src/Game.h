#pragma once

#include "AppWindow.h"
#include "AudioManager.h"
#include "CaptureService.h"
#include "Constants.h"
#include "FramePipeline.h"
#include "InputProvider.h"
#include "Localization.h"
#include "Scene.h"
#include "Settings.h"
#include "compat/RaylibCompat.h"

#include <filesystem>
#include <memory>
#include <string>

namespace wb {

class AssetManager;
class SettingsScene;

namespace render {
class Renderer;
}

class AppWindow;
class AudioBackend;
class CaptureService;
class InputProvider;

enum class SceneId {
    Preloader,
    Settings,
    Quote,
    Gameplay,
    Credits,
    PostCredits,
    Final
};

class SceneManager {
public:
    explicit SceneManager(Game& game);

    void goTo(SceneId id);
    void clear();
    void update(float dt);
    void draw();
    SceneId currentId() const { return currentId_; }

private:
    std::unique_ptr<Scene> make(SceneId id);

    Game& game_;
    std::unique_ptr<Scene> current_;
    SceneId pending_ = SceneId::Preloader;
    SceneId currentId_ = SceneId::Preloader;
    bool hasPending_ = true;
};

struct GameServices {
    std::unique_ptr<AppWindow> window;
    std::unique_ptr<AudioBackend> audioBackend;
    std::unique_ptr<InputProvider> input;
    std::unique_ptr<CaptureService> capture;
    std::unique_ptr<render::Renderer> renderer;
    std::unique_ptr<FramePipeline> framePipeline;
};

class Game {
public:
    Game(std::filesystem::path assetRoot, std::filesystem::path executableDir);
    Game(std::filesystem::path assetRoot, std::filesystem::path executableDir, GameServices services);
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void run();
    void requestScene(SceneId id);
    void requestPauseSettingsClose();
    void requestExit();
    void applySettings(bool resizeWindow = true);
    void stepLanguage(int direction);
    void beginLogicalScissor(Rectangle rect) const;
    int monitorCount() const;
    std::string monitorLabel(int index) const;
    SceneId currentSceneId() const { return scenes_.currentId(); }
    bool exitRequested() const { return exitRequested_; }
    bool pauseMenuOpenedEver() const { return pauseMenuOpenedEver_; }
    bool pauseSettingsOpenedEver() const { return pauseSettingsOpenedEver_; }

    AssetManager& assets() { return *assets_; }
    AudioManager& audio() { return audio_; }
    render::Renderer& renderer() { return *renderer_; }
    const render::Renderer& renderer() const { return *renderer_; }
    InputProvider& input() { return *input_; }
    const InputProvider& input() const { return *input_; }
    CaptureService& capture() { return *capture_; }
    const CaptureService& capture() const { return *capture_; }
    GameSettings& settings() { return settings_; }
    Localization& localization() { return localization_; }
    const Localization& localization() const { return localization_; }
    Vector2 logicalMouse() const { return logicalMouse_; }
    bool logicalMousePressed() const { return input_ && input_->mouseButtonPressed(MouseButton::Left); }
    bool logicalMouseDown() const { return input_ && input_->mouseButtonDown(MouseButton::Left); }
    bool keyPressed(InputKey key) const { return input_ && input_->keyPressed(key); }
    float mouseWheelMove() const { return input_ ? input_->scrollWheelMove() : 0.0f; }
    Rectangle viewport() const { return viewport_; }

private:
    void updateViewport();
    void beginLogicalDraw();
    void endLogicalDraw();
    bool canPauseCurrentScene() const;
    void openPauseMenu();
    void closePauseMenu();
    void openPauseSettings();
    void closePauseSettings();
    void updatePauseMenu();
    void drawPauseMenu();
    Rectangle logicalToRenderRect(Rectangle rect) const;

    std::unique_ptr<AssetManager> assets_;
    AudioManager audio_;
    GameSettings settings_;
    Localization localization_;
    SceneManager scenes_;
    std::unique_ptr<AppWindow> window_;
    std::unique_ptr<InputProvider> input_;
    std::unique_ptr<CaptureService> capture_;
    std::unique_ptr<render::Renderer> renderer_;
    std::unique_ptr<FramePipeline> framePipeline_;
    Rectangle viewport_{};
    Rectangle logicalViewport_{};
    Vector2 logicalMouse_{};
    float logicalScale_ = 1.0f;
    float postTime_ = 0.0f;
    std::unique_ptr<SettingsScene> pauseSettingsScene_;
    bool paused_ = false;
    bool pauseExitConfirm_ = false;
    bool pauseSettingsCloseRequested_ = false;
    bool exitRequested_ = false;
    bool pauseMenuOpenedEver_ = false;
    bool pauseSettingsOpenedEver_ = false;
    int pauseSelected_ = 0;
    int pauseConfirmSelected_ = 0;
};

}  // namespace wb
