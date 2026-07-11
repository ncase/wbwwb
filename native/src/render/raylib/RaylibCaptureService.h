#pragma once

#include "CaptureService.h"

#include "raylib.h"

#include <unordered_map>

namespace wb {

class RaylibCaptureService final : public CaptureService {
public:
    ~RaylibCaptureService() override;

    render::RenderTargetHandle createTarget(std::uint32_t width, std::uint32_t height) override;
    void destroyTarget(render::RenderTargetHandle target) override;
    void beginTarget(render::RenderTargetHandle target) override;
    void endTarget() override;
    CaptureSnapshot snapshot(render::RenderTargetHandle target) override;
    std::vector<std::uint8_t> readTargetRGBA(render::RenderTargetHandle target) override;

private:
    RenderTexture2D* findTarget(render::RenderTargetHandle target);
    static bool pixelsLookBlack(const std::vector<std::uint8_t>& pixels);

    std::unordered_map<std::uint32_t, RenderTexture2D> targets_;
    std::uint32_t nextTargetId_ = 1;
};

}  // namespace wb
