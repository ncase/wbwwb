#pragma once

#include "FramePipeline.h"

namespace wb {

namespace render {
class Renderer;
}

class VulkanFramePipeline final : public FramePipeline {
public:
    explicit VulkanFramePipeline(render::Renderer& renderer);

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
    render::Renderer& renderer_;
    render::Rect viewport_{};
    render::Rect logicalViewport_{};
    float logicalScale_ = 1.0f;
    bool msaaFallbackWarned_ = false;
    float lastPostTime_ = 0.0f;
    float smoothedFps_ = 60.0f;
};

}  // namespace wb
