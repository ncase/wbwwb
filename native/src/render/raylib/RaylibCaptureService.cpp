#include "render/raylib/RaylibCaptureService.h"

#include <cstring>

namespace wb {

RaylibCaptureService::~RaylibCaptureService() {
    for (auto& [_, target] : targets_) {
        if (target.id != 0) {
            UnloadRenderTexture(target);
        }
    }
}

render::RenderTargetHandle RaylibCaptureService::createTarget(std::uint32_t width, std::uint32_t height) {
    RenderTexture2D target = LoadRenderTexture(static_cast<int>(width), static_cast<int>(height));
    if (target.id == 0) {
        return {};
    }
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    const render::RenderTargetHandle handle{nextTargetId_++};
    targets_[handle.id] = target;
    return handle;
}

void RaylibCaptureService::destroyTarget(render::RenderTargetHandle target) {
    auto it = targets_.find(target.id);
    if (it == targets_.end()) {
        return;
    }
    if (it->second.id != 0) {
        UnloadRenderTexture(it->second);
    }
    targets_.erase(it);
}

void RaylibCaptureService::beginTarget(render::RenderTargetHandle target) {
    if (RenderTexture2D* value = findTarget(target)) {
        BeginTextureMode(*value);
    }
}

void RaylibCaptureService::endTarget() {
    EndTextureMode();
}

CaptureSnapshot RaylibCaptureService::snapshot(render::RenderTargetHandle target) {
    RenderTexture2D* value = findTarget(target);
    if (!value || value->texture.id == 0) {
        return {};
    }
    std::vector<std::uint8_t> pixels = readTargetRGBA(target);
    return {
        render::TextureHandle{target.id},
        static_cast<std::uint32_t>(value->texture.width),
        static_cast<std::uint32_t>(value->texture.height),
        !pixels.empty(),
        pixels.empty() || pixelsLookBlack(pixels)
    };
}

std::vector<std::uint8_t> RaylibCaptureService::readTargetRGBA(render::RenderTargetHandle target) {
    RenderTexture2D* value = findTarget(target);
    if (!value || value->texture.id == 0) {
        return {};
    }
    Image image = LoadImageFromTexture(value->texture);
    if (!image.data) {
        return {};
    }
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const std::size_t byteCount = static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4u;
    std::vector<std::uint8_t> pixels(byteCount);
    std::memcpy(pixels.data(), image.data, byteCount);
    UnloadImage(image);
    return pixels;
}

RenderTexture2D* RaylibCaptureService::findTarget(render::RenderTargetHandle target) {
    auto it = targets_.find(target.id);
    if (it == targets_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool RaylibCaptureService::pixelsLookBlack(const std::vector<std::uint8_t>& pixels) {
    for (std::size_t i = 0; i + 3 < pixels.size(); i += 4) {
        if (pixels[i + 3] > 8 && (pixels[i] > 8 || pixels[i + 1] > 8 || pixels[i + 2] > 8)) {
            return false;
        }
    }
    return true;
}

}  // namespace wb
