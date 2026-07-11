#include "render/vulkan/VulkanCaptureService.h"

namespace wb {

render::RenderTargetHandle VulkanCaptureService::createTarget(std::uint32_t width, std::uint32_t height) {
    return renderer_.CreateRenderTarget(width, height);
}

void VulkanCaptureService::destroyTarget(render::RenderTargetHandle target) {
    renderer_.DestroyRenderTarget(target);
}

void VulkanCaptureService::beginTarget(render::RenderTargetHandle target) {
    renderer_.BeginRenderTarget(target);
}

void VulkanCaptureService::endTarget() {
    renderer_.EndRenderTarget();
}

CaptureSnapshot VulkanCaptureService::snapshot(render::RenderTargetHandle target) {
    const render::TextureHandle texture = renderer_.RenderTargetTexture(target);
    const render::Vec2 size = renderer_.TextureSize(texture);
    const std::vector<std::uint8_t> pixels = renderer_.CaptureRenderTargetRGBA(target);
    return {
        texture,
        static_cast<std::uint32_t>(size.x > 0.0f ? size.x : 0.0f),
        static_cast<std::uint32_t>(size.y > 0.0f ? size.y : 0.0f),
        !pixels.empty(),
        pixels.empty() || pixelsLookBlack(pixels)
    };
}

std::vector<std::uint8_t> VulkanCaptureService::readTargetRGBA(render::RenderTargetHandle target) {
    return renderer_.CaptureRenderTargetRGBA(target);
}

bool VulkanCaptureService::pixelsLookBlack(const std::vector<std::uint8_t>& pixels) {
    for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
        if (pixels[i + 3] > 8 && (pixels[i] > 8 || pixels[i + 1] > 8 || pixels[i + 2] > 8)) {
            return false;
        }
    }
    return true;
}

}  // namespace wb
