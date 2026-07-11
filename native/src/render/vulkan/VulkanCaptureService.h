#pragma once

#include "CaptureService.h"

namespace wb {

class VulkanCaptureService final : public CaptureService {
public:
    explicit VulkanCaptureService(render::Renderer& renderer) : renderer_(renderer) {}

    render::RenderTargetHandle createTarget(std::uint32_t width, std::uint32_t height) override;
    void destroyTarget(render::RenderTargetHandle target) override;
    void beginTarget(render::RenderTargetHandle target) override;
    void endTarget() override;
    CaptureSnapshot snapshot(render::RenderTargetHandle target) override;
    std::vector<std::uint8_t> readTargetRGBA(render::RenderTargetHandle target) override;

private:
    static bool pixelsLookBlack(const std::vector<std::uint8_t>& pixels);

    render::Renderer& renderer_;
};

}  // namespace wb
