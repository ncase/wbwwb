#include "Game.h"

#include "AssetManager.h"
#include "Scenes.h"
#include "render/RenderBackend.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace wb {
namespace {

constexpr float kMaxFrameTime = 1.0f / 15.0f;

constexpr render::Rect kPausePanel{300.0f, 105.0f, 360.0f, 330.0f};
constexpr std::array<render::Rect, 3> kPauseButtons{{
    {335.0f, 205.0f, 290.0f, 54.0f},
    {335.0f, 271.0f, 290.0f, 54.0f},
    {335.0f, 337.0f, 290.0f, 54.0f}
}};
constexpr std::array<render::Rect, 2> kPauseConfirmButtons{{
    {325.0f, 315.0f, 145.0f, 52.0f},
    {490.0f, 315.0f, 145.0f, 52.0f}
}};

bool contains(render::Rect rect, Vector2 point) {
    return point.x >= rect.x && point.y >= rect.y &&
           point.x <= rect.x + rect.width && point.y <= rect.y + rect.height;
}

void drawCenteredText(render::Renderer& renderer,
                      render::FontHandle font,
                      std::string_view text,
                      float centerX,
                      float centerY,
                      float size,
                      render::Color color) {
    const render::Vec2 measured = renderer.MeasureText(font, text, size);
    renderer.DrawText({
        font,
        text,
        {centerX - measured.x * 0.5f, centerY - measured.y * 0.5f},
        size,
        color,
        1.0f
    });
}

int frameLimitFps(int frameLimit) {
    switch (frameLimit) {
        case 0: return 30;
        case 2: return 120;
        case 3: return 0;
        default: return 60;
    }
}

render::OutputColorMode outputColorModeFor(int hdrMode) {
    if (hdrMode >= 2) {
        return render::OutputColorMode::Hdr;
    }
    if (hdrMode == 1) {
        return render::OutputColorMode::HdrAuto;
    }
    return render::OutputColorMode::Sdr;
}

int clampedMonitorIndex(int index, int monitorCount) {
    const int count = std::max(1, monitorCount);
    return std::clamp(index, 0, count - 1);
}

std::filesystem::path normalizedPath(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(path, ec);
    if (!ec) {
        return normalized;
    }
    return std::filesystem::absolute(path, ec).lexically_normal();
}

void appendUniquePath(std::vector<std::filesystem::path>& paths, const std::filesystem::path& path) {
    const std::filesystem::path normalized = normalizedPath(path);
    if (std::find(paths.begin(), paths.end(), normalized) == paths.end()) {
        paths.push_back(normalized);
    }
}

void ensureTranslationDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        std::cerr << "Warning: could not create translations folder " << path << ": " << ec.message() << "\n";
    }
}

std::vector<std::filesystem::path> fanTranslationDirectories(const std::filesystem::path& executableDir, const std::filesystem::path& assetRoot) {
    std::vector<std::filesystem::path> createdDirectories;
    appendUniquePath(createdDirectories, executableDir / "translations");
    appendUniquePath(createdDirectories, std::filesystem::current_path() / "translations");
    for (const auto& directory : createdDirectories) {
        ensureTranslationDirectory(directory);
    }

    std::vector<std::filesystem::path> directories = createdDirectories;
    appendUniquePath(directories, assetRoot / "translations");
    return directories;
}

}  // namespace

SceneManager::SceneManager(Game& game) : game_(game) {}

void SceneManager::goTo(SceneId id) {
    pending_ = id;
    hasPending_ = true;
}

void SceneManager::clear() {
    if (current_) {
        current_->exit();
    }
    current_.reset();
    hasPending_ = false;
}

void SceneManager::update(float dt) {
    if (hasPending_) {
        if (current_) {
            current_->exit();
        }
        current_ = make(pending_);
        currentId_ = pending_;
        current_->enter();
        hasPending_ = false;
    }

    if (current_) {
        current_->update(dt);
    }
}

void SceneManager::draw() {
    if (current_) {
        current_->draw();
    }
}

std::unique_ptr<Scene> SceneManager::make(SceneId id) {
    switch (id) {
        case SceneId::Preloader:
            return std::make_unique<PreloaderScene>(game_);
        case SceneId::Settings:
            return std::make_unique<SettingsScene>(game_);
        case SceneId::Quote:
            return std::make_unique<QuoteScene>(game_);
        case SceneId::Gameplay:
            return std::make_unique<GameplayScene>(game_);
        case SceneId::Credits:
            return std::make_unique<CreditsScene>(game_);
        case SceneId::PostCredits:
            return std::make_unique<PostCreditsScene>(game_);
        case SceneId::Final:
            return std::make_unique<FinalScene>(game_);
    }
    throw std::runtime_error("Unknown scene id");
}

Game::Game(std::filesystem::path assetRoot, std::filesystem::path executableDir, GameServices services)
    : assets_(std::make_unique<AssetManager>(std::move(assetRoot))),
      audio_(std::move(services.audioBackend)),
      scenes_(*this),
      window_(std::move(services.window)),
      input_(std::move(services.input)),
      capture_(std::move(services.capture)),
      renderer_(std::move(services.renderer)),
      framePipeline_(std::move(services.framePipeline)) {
    if (!window_ || !input_ || !capture_ || !renderer_ || !framePipeline_) {
        throw std::invalid_argument("Game requires complete backend services");
    }

    settings_.load();
    localization_.load(
        assets_->path("native/assets/lang"),
        fanTranslationDirectories(executableDir, assets_->root()),
        settings_.languageId
    );
    if (settings_.languageId != localization_.currentLanguageId()) {
        settings_.languageId = localization_.currentLanguageId();
        settings_.save();
    }

    const WindowSizeOption& window = settings_.currentWindowSize();
    window_->initialize(settings_, kTitle);

    render::RendererConfig rendererConfig{};
    rendererConfig.logicalWidth = kGameWidth;
    rendererConfig.logicalHeight = kGameHeight;
    rendererConfig.renderWidth = static_cast<std::uint32_t>(std::max(1, window.width));
    rendererConfig.renderHeight = static_cast<std::uint32_t>(std::max(1, window.height));
    rendererConfig.outputWidth = static_cast<std::uint32_t>(std::max(1, window.width));
    rendererConfig.outputHeight = static_cast<std::uint32_t>(std::max(1, window.height));
    rendererConfig.vsync = settings_.vsync;
    rendererConfig.outputFilter = settings_.outputSmoothing == 1 ? render::TextureFilter::Linear : render::TextureFilter::Nearest;
    rendererConfig.requestedOutputColorMode = outputColorModeFor(settings_.hdrMode);
    rendererConfig.msaaSamples = static_cast<std::uint32_t>(std::max(1, settings_.msaaSamples));
    renderer_->Initialize(rendererConfig);
    assets_->setRenderer(renderer_.get());
    RaylibCompatContext compatContext{};
    compatContext.renderer = renderer_.get();
    compatContext.input = input_.get();
    compatContext.audio = &audio_;
    compatContext.capture = capture_.get();
    compatContext.assets = assets_.get();
    compatContext.framePipeline = framePipeline_.get();
    compatContext.localization = &localization_;
    compatContext.settings = &settings_;
    setRaylibCompatContext(compatContext);
    std::cout << "Renderer backend: " << configuredRenderBackendName() << "\n";
    window_->applyIcons(assets_->path("sprites/app_icon.png"));
    std::vector<WindowSizeOption> windowSizes = detectSystemWindowSizeOptions();
    std::vector<WindowSizeOption> monitorSizes = window_->monitorWindowSizes();
    windowSizes.insert(windowSizes.end(), monitorSizes.begin(), monitorSizes.end());
    settings_.refreshWindowSizeOptions(windowSizes);
    window_->setTargetFps(frameLimitFps(settings_.frameLimit));
    applySettings(true);
}

Game::~Game() {
    closePauseSettings();
    scenes_.clear();
    clearRaylibCompatContext();
    assets_->setRenderer(nullptr);
    framePipeline_.reset();
    renderer_.reset();
    capture_.reset();
    input_.reset();
    audio_.shutdown();
    if (window_) {
        window_->shutdown();
    }
}

void Game::requestScene(SceneId id) {
    scenes_.goTo(id);
}

void Game::requestPauseSettingsClose() {
    pauseSettingsCloseRequested_ = true;
}

void Game::requestExit() {
    exitRequested_ = true;
}

int Game::monitorCount() const {
    return window_ ? std::max(1, window_->monitorCount()) : 1;
}

std::string Game::monitorLabel(int index) const {
    const int monitor = std::clamp(index, 0, monitorCount() - 1);
    const std::string name = window_ ? window_->monitorName(monitor) : std::string();
    if (!name.empty()) {
        return name;
    }
    return localization_.tr("settings.value.monitor_number") + " " + std::to_string(monitor + 1);
}

void Game::applySettings(bool resizeWindow) {
    audio_.setVolumeMix(settings_.masterVolume, settings_.musicVolume, settings_.sfxVolume);
    assets_->setSmoothTextures(settings_.outputSmoothing == 1);
    window_->setTargetFps(frameLimitFps(settings_.frameLimit));

    window_->setVsync(settings_.vsync);

    settings_.clampMonitorIndex(monitorCount());
    const int monitor = clampedMonitorIndex(settings_.monitorIndex, monitorCount());
    const int monitorWidth = std::max(640, window_->monitorWidth(monitor));
    const int monitorHeight = std::max(360, window_->monitorHeight(monitor));
    const render::Vec2 monitorPos = window_->monitorPosition(monitor);
    if (settings_.windowScale != 5) {
        settings_.syncWindowSizeToScale(monitorWidth, monitorHeight);
    }

    const bool currentlyFullscreen = window_->isFullscreen();
    if (settings_.displayMode != 2 && currentlyFullscreen) {
        window_->setFullscreen(false);
    }

    if (settings_.displayMode == 2) {
        window_->setUndecorated(false);
        if (!currentlyFullscreen || window_->currentMonitor() != monitor) {
            if (currentlyFullscreen) {
                window_->setFullscreen(false);
            }
            window_->setWindowPosition(static_cast<int>(monitorPos.x), static_cast<int>(monitorPos.y));
            window_->setWindowSize(monitorWidth, monitorHeight);
            window_->setFullscreen(true);
        }
        settings_.fullscreen = true;
    } else if (settings_.displayMode == 1) {
        window_->setUndecorated(true);
        window_->setWindowPosition(static_cast<int>(monitorPos.x), static_cast<int>(monitorPos.y));
        window_->setWindowSize(monitorWidth, monitorHeight);
        settings_.fullscreen = false;
    } else {
        window_->setUndecorated(false);
        if (resizeWindow) {
            const WindowSizeOption& window = settings_.currentWindowSize();
            window_->setWindowSize(window.width, window.height);
            const int centeredX = static_cast<int>(monitorPos.x) + std::max(0, (monitorWidth - window.width) / 2);
            const int centeredY = static_cast<int>(monitorPos.y) + std::max(0, (monitorHeight - window.height) / 2);
            window_->setWindowPosition(centeredX, centeredY);
        }
        settings_.fullscreen = false;
    }

    updateViewport();
    if (renderer_) {
        const WindowSizeOption& renderResolution = settings_.currentWindowSize();
        renderer_->SetRenderResolution(
            static_cast<std::uint32_t>(std::max(1, renderResolution.width)),
            static_cast<std::uint32_t>(std::max(1, renderResolution.height))
        );
        assets_->updateRenderResolution(
            static_cast<std::uint32_t>(std::max(1, renderResolution.width)),
            static_cast<std::uint32_t>(std::max(1, renderResolution.height))
        );
        renderer_->Resize(static_cast<std::uint32_t>(std::max(1, window_->clientWidth())),
                          static_cast<std::uint32_t>(std::max(1, window_->clientHeight())));
        renderer_->SetOutputFilter(settings_.outputSmoothing == 1 ? render::TextureFilter::Linear : render::TextureFilter::Nearest);
        renderer_->SetOutputColorMode(outputColorModeFor(settings_.hdrMode));
        renderer_->SetMsaaSamples(static_cast<std::uint32_t>(std::max(1, settings_.msaaSamples)));
    }
}

void Game::stepLanguage(int direction) {
    const std::string previous = localization_.currentLanguageId();
    localization_.stepLanguage(direction);
    if (localization_.currentLanguageId() != previous || settings_.languageId != localization_.currentLanguageId()) {
        settings_.languageId = localization_.currentLanguageId();
        settings_.save();
    }
}

void Game::beginLogicalScissor(Rectangle rect) const {
    if (renderer_) {
        renderer_->PushScissor({rect.x, rect.y, rect.width, rect.height});
    }
}

void Game::run() {
    while (!exitRequested_ && !input_->windowCloseRequested()) {
        const float dt = std::min(window_->frameTime(), kMaxFrameTime);
        postTime_ += dt;
        updateViewport();
        audio_.update(dt);

        if (pauseSettingsScene_) {
            pauseSettingsScene_->update(dt);
            if (pauseSettingsCloseRequested_) {
                closePauseSettings();
            }
        } else if (paused_) {
            updatePauseMenu();
        } else if (canPauseCurrentScene() && keyPressed(InputKey::Escape)) {
            openPauseMenu();
        } else {
            scenes_.update(dt);
        }

        beginLogicalDraw();
        scenes_.draw();
        if (pauseSettingsScene_) {
            pauseSettingsScene_->draw();
        } else if (paused_) {
            drawPauseMenu();
        }
        endLogicalDraw();
    }
}

bool Game::canPauseCurrentScene() const {
    const SceneId id = scenes_.currentId();
    return id != SceneId::Preloader && id != SceneId::Settings;
}

void Game::openPauseMenu() {
    paused_ = true;
    pauseExitConfirm_ = false;
    pauseSelected_ = 0;
    pauseConfirmSelected_ = 0;
    pauseMenuOpenedEver_ = true;
}

void Game::closePauseMenu() {
    closePauseSettings();
    paused_ = false;
    pauseExitConfirm_ = false;
}

void Game::openPauseSettings() {
    if (pauseSettingsScene_) {
        return;
    }
    pauseSettingsCloseRequested_ = false;
    pauseSettingsScene_ = std::make_unique<SettingsScene>(*this, SettingsScene::ReturnTarget::PauseMenu);
    pauseSettingsScene_->enter();
    pauseSettingsOpenedEver_ = true;
}

void Game::closePauseSettings() {
    if (pauseSettingsScene_) {
        pauseSettingsScene_->exit();
        pauseSettingsScene_.reset();
    }
    pauseSettingsCloseRequested_ = false;
}

void Game::updatePauseMenu() {
    if (keyPressed(InputKey::Escape)) {
        if (pauseExitConfirm_) {
            pauseExitConfirm_ = false;
            pauseConfirmSelected_ = 0;
        } else {
            closePauseMenu();
        }
        return;
    }

    const Vector2 mouse = logicalMouse();
    if (pauseExitConfirm_) {
        for (int i = 0; i < static_cast<int>(kPauseConfirmButtons.size()); ++i) {
            if (contains(kPauseConfirmButtons[static_cast<std::size_t>(i)], mouse)) {
                pauseConfirmSelected_ = i;
            }
        }
        if (keyPressed(InputKey::Left) || keyPressed(InputKey::Right) ||
            keyPressed(InputKey::Up) || keyPressed(InputKey::Down)) {
            pauseConfirmSelected_ = 1 - pauseConfirmSelected_;
            audio_.playSound("sounds/squeak.mp3", 0.35f);
        }
        const bool activate = keyPressed(InputKey::Enter) || keyPressed(InputKey::Space) ||
            (logicalMousePressed() && contains(kPauseConfirmButtons[static_cast<std::size_t>(pauseConfirmSelected_)], mouse));
        if (activate) {
            audio_.playSound("sounds/squeak.mp3", 0.5f);
            if (pauseConfirmSelected_ == 0) {
                pauseExitConfirm_ = false;
            } else {
                requestExit();
            }
        }
        return;
    }

    for (int i = 0; i < static_cast<int>(kPauseButtons.size()); ++i) {
        if (contains(kPauseButtons[static_cast<std::size_t>(i)], mouse)) {
            pauseSelected_ = i;
        }
    }
    if (keyPressed(InputKey::Down)) {
        pauseSelected_ = (pauseSelected_ + 1) % static_cast<int>(kPauseButtons.size());
        audio_.playSound("sounds/squeak.mp3", 0.35f);
    }
    if (keyPressed(InputKey::Up)) {
        pauseSelected_ = (pauseSelected_ + static_cast<int>(kPauseButtons.size()) - 1) % static_cast<int>(kPauseButtons.size());
        audio_.playSound("sounds/squeak.mp3", 0.35f);
    }

    const bool activate = keyPressed(InputKey::Enter) || keyPressed(InputKey::Space) ||
        (logicalMousePressed() && contains(kPauseButtons[static_cast<std::size_t>(pauseSelected_)], mouse));
    if (!activate) {
        return;
    }
    audio_.playSound("sounds/squeak.mp3", 0.5f);
    if (pauseSelected_ == 0) {
        closePauseMenu();
    } else if (pauseSelected_ == 1) {
        openPauseSettings();
    } else {
        pauseExitConfirm_ = true;
        pauseConfirmSelected_ = 0;
    }
}

void Game::drawPauseMenu() {
    render::Renderer& renderer = *renderer_;
    const render::FontHandle font = assets_->fontHandle(64);
    renderer.DrawRectangle({0.0f, 0.0f, static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)}, {0, 0, 0, 255}, 0.68f);
    renderer.DrawRectangle(kPausePanel, {20, 20, 22, 248});
    renderer.DrawRectangleOutline(kPausePanel, 3.0f, {226, 226, 226, 255});
    renderer.DrawRectangle({kPausePanel.x, kPausePanel.y, kPausePanel.width, 70.0f}, {205, 31, 35, 255});
    drawCenteredText(renderer, font, localization_.tr("pause.title"), 480.0f, 140.0f, 36.0f, {255, 255, 255, 255});

    const std::array<std::string, 3> labels{{
        localization_.tr("pause.resume"),
        localization_.tr("pause.settings"),
        localization_.tr("pause.exit")
    }};
    for (int i = 0; i < static_cast<int>(kPauseButtons.size()); ++i) {
        const bool selected = pauseSelected_ == i && !pauseExitConfirm_;
        const render::Rect button = kPauseButtons[static_cast<std::size_t>(i)];
        renderer.DrawRectangle(button, selected ? render::Color{205, 31, 35, 255} : render::Color{48, 48, 51, 245});
        renderer.DrawRectangleOutline(button, selected ? 3.0f : 2.0f, selected ? render::Color{255, 255, 255, 255} : render::Color{118, 118, 122, 255});
        drawCenteredText(renderer, font, labels[static_cast<std::size_t>(i)], button.x + button.width * 0.5f, button.y + button.height * 0.5f, 25.0f, {255, 255, 255, 255});
    }

    if (!pauseExitConfirm_) {
        return;
    }

    const render::Rect confirm{285.0f, 178.0f, 390.0f, 210.0f};
    renderer.DrawRectangle(confirm, {14, 14, 16, 252});
    renderer.DrawRectangleOutline(confirm, 3.0f, {255, 255, 255, 255});
    drawCenteredText(renderer, font, localization_.tr("pause.exit_confirm"), 480.0f, 240.0f, 25.0f, {255, 255, 255, 255});
    const std::array<std::string, 2> confirmLabels{{
        localization_.tr("pause.cancel"),
        localization_.tr("pause.confirm_exit")
    }};
    for (int i = 0; i < static_cast<int>(kPauseConfirmButtons.size()); ++i) {
        const bool selected = pauseConfirmSelected_ == i;
        const render::Rect button = kPauseConfirmButtons[static_cast<std::size_t>(i)];
        renderer.DrawRectangle(button, selected ? render::Color{205, 31, 35, 255} : render::Color{48, 48, 51, 255});
        renderer.DrawRectangleOutline(button, selected ? 3.0f : 2.0f, selected ? render::Color{255, 255, 255, 255} : render::Color{118, 118, 122, 255});
        drawCenteredText(renderer, font, confirmLabels[static_cast<std::size_t>(i)], button.x + button.width * 0.5f, button.y + button.height * 0.5f, 20.0f, {255, 255, 255, 255});
    }
}

void Game::updateViewport() {
    if (!framePipeline_) {
        return;
    }
    framePipeline_->UpdateViewport(settings_);
    const render::Rect viewport = framePipeline_->Viewport();
    const render::Rect logicalViewport = framePipeline_->LogicalViewport();
    viewport_ = {viewport.x, viewport.y, viewport.width, viewport.height};
    logicalViewport_ = {logicalViewport.x, logicalViewport.y, logicalViewport.width, logicalViewport.height};
    logicalScale_ = framePipeline_->LogicalScale();

    input_->updateLogicalMapping(
        viewport,
        {static_cast<float>(kGameWidth), static_cast<float>(kGameHeight)}
    );
    const render::Vec2 mouse = input_->logicalMousePosition();
    logicalMouse_ = {mouse.x, mouse.y};
}

void Game::beginLogicalDraw() {
    framePipeline_->BeginLogicalDraw(settings_);
}

void Game::endLogicalDraw() {
    framePipeline_->EndLogicalDraw(settings_, assets_.get(), &localization_, postTime_);
}

Rectangle Game::logicalToRenderRect(Rectangle rect) const {
    return {
        logicalViewport_.x + rect.x * logicalScale_,
        logicalViewport_.y + rect.y * logicalScale_,
        rect.width * logicalScale_,
        rect.height * logicalScale_
    };
}

}  // namespace wb
