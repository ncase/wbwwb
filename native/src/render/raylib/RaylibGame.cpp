#include "Game.h"

#include "render/raylib/RaylibAppWindow.h"
#include "render/raylib/RaylibAudioBackend.h"
#include "render/raylib/RaylibCaptureService.h"
#include "render/raylib/RaylibFramePipeline.h"
#include "render/raylib/RaylibInputProvider.h"
#include "render/raylib/RaylibRenderer.h"

#include <utility>

namespace wb {
namespace {

GameServices createRaylibServices(const std::filesystem::path& assetRoot) {
    GameServices services;
    services.window = std::make_unique<RaylibAppWindow>();
    services.audioBackend = std::make_unique<RaylibAudioBackend>(assetRoot);
    services.input = std::make_unique<RaylibInputProvider>();
    services.capture = std::make_unique<RaylibCaptureService>();
    auto renderer = std::make_unique<RaylibRenderer>();
    RaylibRenderer* rendererPtr = renderer.get();
    services.renderer = std::move(renderer);
    services.framePipeline = std::make_unique<RaylibFramePipeline>(*rendererPtr);
    return services;
}

}  // namespace

Game::Game(std::filesystem::path assetRoot, std::filesystem::path executableDir)
    : Game(assetRoot, std::move(executableDir), createRaylibServices(assetRoot)) {}

}  // namespace wb
