#include "render/vulkan/VulkanFramePipeline.h"

#include "AssetManager.h"
#include "Constants.h"
#include "Localization.h"
#include "render/Renderer.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdio>

namespace wb {
namespace {

constexpr float kMinViewportScale = 0.001f;

}  // namespace

VulkanFramePipeline::VulkanFramePipeline(render::Renderer& renderer)
    : renderer_(renderer) {
}

void VulkanFramePipeline::UpdateViewport(const GameSettings& settings) {
    renderer_.SetOutputFilter(settings.outputSmoothing == 1 ? render::TextureFilter::Linear : render::TextureFilter::Nearest);
    const render::RendererStats stats = renderer_.Stats();
    const render::Vec2 logicalSize = renderer_.LogicalSize();
    const float logicalWidth = logicalSize.x > 0.0f ? logicalSize.x : static_cast<float>(kGameWidth);
    const float logicalHeight = logicalSize.y > 0.0f ? logicalSize.y : static_cast<float>(kGameHeight);
    const float outputWidth = static_cast<float>(std::max(1u, stats.outputWidth));
    const float outputHeight = static_cast<float>(std::max(1u, stats.outputHeight));
    const float safeArea = std::clamp(settings.safeArea, 0.9f, 1.0f);
    const float areaWidth = std::max(1.0f, outputWidth * safeArea);
    const float areaHeight = std::max(1.0f, outputHeight * safeArea);
    const float areaX = (outputWidth - areaWidth) * 0.5f;
    const float areaY = (outputHeight - areaHeight) * 0.5f;
    const float scaleX = areaWidth / logicalWidth;
    const float scaleY = areaHeight / logicalHeight;
    float presentationScale = settings.aspectFit == 1 ? std::max(scaleX, scaleY) : std::min(scaleX, scaleY);
    if (settings.integerScaling && presentationScale >= 1.0f) {
        presentationScale = settings.aspectFit == 1 ? std::ceil(presentationScale) : std::floor(presentationScale);
    }
    presentationScale = std::max(kMinViewportScale, presentationScale);
    viewport_.width = std::max(1.0f, std::round(logicalWidth * presentationScale));
    viewport_.height = std::max(1.0f, std::round(logicalHeight * presentationScale));
    viewport_.x = std::round(areaX + (areaWidth - viewport_.width) * 0.5f);
    viewport_.y = std::round(areaY + (areaHeight - viewport_.height) * 0.5f);
    renderer_.SetFinalPresentationRect(viewport_);

    logicalViewport_ = {0.0f, 0.0f, logicalWidth, logicalHeight};
    logicalScale_ = 1.0f;
}

void VulkanFramePipeline::BeginLogicalDraw(const GameSettings& settings) {
    UpdateViewport(settings);
    renderer_.BeginFrame();
    renderer_.BeginLogicalRender();
}

void VulkanFramePipeline::EndLogicalDraw(const GameSettings& settings,
                                         AssetManager* assets,
                                         const Localization* localization,
                                         float postTime) {
    const float flashScale = settings.reduceFlashing ? 0.35f : 1.0f;
    render::PostProcessSettings post;
    post.fxaa = settings.fxaa;
    post.crtStrength = (settings.crtFilter == 2 ? 0.78f : settings.crtFilter == 1 ? 0.38f : 0.0f) * flashScale;
    post.noiseAmount = (settings.screenNoise == 3 ? 0.10f : settings.screenNoise == 2 ? 0.055f : settings.screenNoise == 1 ? 0.025f : 0.0f) * flashScale;
    post.brightness = std::clamp(settings.brightness, 0.5f, 1.5f);
    post.contrast = std::clamp(settings.contrast, 0.5f, 1.5f);
    post.sharpness = std::clamp(settings.sharpness, 0.0f, 2.0f);
    post.gamma = std::clamp(settings.gamma, 0.8f, 1.2f);
    post.blackLevel = std::clamp(settings.blackLevel, 0.0f, 0.2f);
    post.whiteLevel = std::clamp(settings.whiteLevel, 0.8f, 1.2f);
    post.screenBorder = settings.screenBorder == 2 ? 0.65f : settings.screenBorder == 1 ? 0.32f : 0.0f;
    post.scanlines = (settings.scanlines == 3 ? 0.34f : settings.scanlines == 2 ? 0.22f : settings.scanlines == 1 ? 0.12f : 0.0f) * flashScale;
    post.crtCurve = settings.crtCurve == 2 ? 0.11f : settings.crtCurve == 1 ? 0.055f : 0.0f;
    post.hdrPaperWhiteNits = std::clamp(settings.hdrPaperWhiteNits, 80.0f, 500.0f);
    post.hdrPeakNits = std::clamp(settings.hdrPeakNits, 400.0f, 4000.0f);
    post.hdrHighlightStrength = std::clamp(settings.hdrHighlightStrength, 0.0f, 1.0f);
    post.time = postTime;
    renderer_.SetPostProcessSettings(post);
    const std::uint32_t requestedSamples = static_cast<std::uint32_t>(std::max(1, settings.msaaSamples));
    const std::uint32_t activeSamples = renderer_.Stats().msaaSamples;
    if (requestedSamples > activeSamples && !msaaFallbackWarned_) {
        std::cerr << "Warning: requested " << requestedSamples << "x Vulkan MSAA, but this GPU selected "
                  << activeSamples << "x.\n";
        msaaFallbackWarned_ = true;
    }
    const float dt = postTime - lastPostTime_;
    if (lastPostTime_ > 0.0f && dt > 0.00001f && dt < 1.0f) {
        const float instantaneous = 1.0f / dt;
        smoothedFps_ = smoothedFps_ * 0.9f + instantaneous * 0.1f;
    }
    lastPostTime_ = postTime;
    if (settings.fpsCounter && assets && localization) {
        char buffer[48]{};
        std::snprintf(buffer,
                      sizeof(buffer),
                      "%s: %d",
                      localization->tr("hud.fps").c_str(),
                      static_cast<int>(std::lround(smoothedFps_)));
        renderer_.DrawText({
            assets->fontHandle(64),
            buffer,
            {8.0f, 7.0f},
            16.0f,
            {235, 235, 235, 230},
            1.0f
        });
    }

    renderer_.EndLogicalRender();
    renderer_.Present();
    renderer_.EndFrame();
}

}  // namespace wb
