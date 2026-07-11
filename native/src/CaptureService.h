#pragma once

#include "render/Renderer.h"

#include <cstdint>
#include <vector>

namespace wb {

struct CaptureSnapshot {
    render::TextureHandle texture{};
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    bool valid = false;
    bool looksBlack = true;
};

class CaptureService {
public:
    virtual ~CaptureService() = default;

    virtual render::RenderTargetHandle createTarget(std::uint32_t width, std::uint32_t height) = 0;
    virtual void destroyTarget(render::RenderTargetHandle target) = 0;
    virtual void beginTarget(render::RenderTargetHandle target) = 0;
    virtual void endTarget() = 0;
    virtual CaptureSnapshot snapshot(render::RenderTargetHandle target) = 0;
    virtual std::vector<std::uint8_t> readTargetRGBA(render::RenderTargetHandle target) = 0;
};

}  // namespace wb
