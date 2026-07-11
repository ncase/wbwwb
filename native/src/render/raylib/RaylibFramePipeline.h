#pragma once

#include "FramePipeline.h"

#include "raylib.h"

namespace wb {

class RaylibRenderer;

class RaylibFramePipeline final : public FramePipeline {
public:
    explicit RaylibFramePipeline(RaylibRenderer& renderer);
    ~RaylibFramePipeline() override;

    void UpdateViewport(const GameSettings& settings) override;
    void BeginLogicalDraw(const GameSettings& settings) override;
    void EndLogicalDraw(const GameSettings& settings,
                        AssetManager* assets,
                        const Localization* localization,
                        float postTime) override;

    render::Rect Viewport() const override { return viewport_; }
    render::Rect LogicalViewport() const override { return logicalViewport_; }
    float LogicalScale() const override { return logicalScale_; }

private:
    struct MsaaRenderTarget {
        unsigned int framebuffer = 0;
        unsigned int colorBuffer = 0;
        int width = 0;
        int height = 0;
        int samples = 0;
        int requestedSamples = 0;
    };

    void UpdateRenderTarget(const GameSettings& settings);
    void UpdateMsaaTarget(const GameSettings& settings);
    bool LoadMsaaTarget(int width, int height, int requestedSamples);
    void UnloadMsaaTarget();
    bool UsingMsaaTarget() const;
    void ResolveMsaaTarget();
    bool PostProcessingRequested(const GameSettings& settings) const;
    bool EnsurePostShader(AssetManager& assets);
    void UnloadPostShader();
    void UpdatePostShaderUniforms(const GameSettings& settings, float postTime);

    RaylibRenderer& renderer_;
    RenderTexture2D target_{};
    MsaaRenderTarget msaaTarget_{};
    Shader postShader_{};
    bool postShaderTried_ = false;
    bool postShaderReady_ = false;
    int postTextureSizeLoc_ = -1;
    int postTimeLoc_ = -1;
    int postUseFxaaLoc_ = -1;
    int postCrtStrengthLoc_ = -1;
    int postNoiseAmountLoc_ = -1;
    int postBrightnessLoc_ = -1;
    int postContrastLoc_ = -1;
    int postSharpnessLoc_ = -1;
    int postGammaLoc_ = -1;
    int postBlackLevelLoc_ = -1;
    int postWhiteLevelLoc_ = -1;
    int postScreenBorderLoc_ = -1;
    int postScanlinesLoc_ = -1;
    int postCrtCurveLoc_ = -1;
    render::Rect viewport_{};
    render::Rect logicalViewport_{};
    int renderWidth_ = 960;
    int renderHeight_ = 540;
    float logicalScale_ = 1.0f;
    bool msaaUnavailableWarned_ = false;
};

}  // namespace wb
