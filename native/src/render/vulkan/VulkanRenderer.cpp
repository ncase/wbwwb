#include "render/vulkan/VulkanRenderer.h"

#include <algorithm>
#include <stdexcept>

namespace wb {
namespace {

wbwwb::vulkan::VulkanHdrMode toVulkanHdrMode(render::OutputColorMode mode) {
    switch (mode) {
        case render::OutputColorMode::Hdr:
            return wbwwb::vulkan::VulkanHdrMode::On;
        case render::OutputColorMode::HdrAuto:
            return wbwwb::vulkan::VulkanHdrMode::Auto;
        case render::OutputColorMode::Sdr:
            return wbwwb::vulkan::VulkanHdrMode::Off;
    }
    return wbwwb::vulkan::VulkanHdrMode::Off;
}

} // namespace

VulkanRenderer::VulkanRenderer(wbwwb::vulkan::VulkanBackend& backend)
    : backend_(backend) {
}

VulkanRenderer::~VulkanRenderer() {
    Shutdown();
}

void VulkanRenderer::Initialize(const render::RendererConfig& config) {
    config_ = config;
    config_.logicalWidth = 960;
    config_.logicalHeight = 540;
    config_.renderWidth = std::max(1u, config_.renderWidth);
    config_.renderHeight = std::max(1u, config_.renderHeight);
    backend_.SetRenderResolution(config_.renderWidth, config_.renderHeight);
    backend_.SetPresentationOptions(config_.vsync, config_.outputFilter == render::TextureFilter::Linear);
    backend_.SetHdrMode(toVulkanHdrMode(config_.requestedOutputColorMode));
    backend_.SetMsaaSamples(config_.msaaSamples);
    initialized_ = true;
}

void VulkanRenderer::Shutdown() {
    if (!initialized_) {
        return;
    }

    backend_.WaitIdle();

    for (std::uint32_t textureId : textureHandles_) {
        backend_.DestroyRendererTexture(render::TextureHandle{textureId});
    }
    textureHandles_.clear();
    targetTextureOwners_.clear();

    for (std::uint32_t fontId : fontHandles_) {
        backend_.DestroyRendererFont(render::FontHandle{fontId});
    }
    fontHandles_.clear();

    for (std::uint32_t targetId : targetHandles_) {
        backend_.DestroyRendererRenderTarget(render::RenderTargetHandle{targetId});
    }
    targetHandles_.clear();

    initialized_ = false;
}

void VulkanRenderer::Resize(std::uint32_t outputWidth, std::uint32_t outputHeight) {
    if (config_.outputWidth == outputWidth && config_.outputHeight == outputHeight) {
        return;
    }
    config_.outputWidth = outputWidth;
    config_.outputHeight = outputHeight;
    backend_.Resize(outputWidth, outputHeight);
}

void VulkanRenderer::SetRenderResolution(std::uint32_t renderWidth, std::uint32_t renderHeight) {
    renderWidth = std::max(1u, renderWidth);
    renderHeight = std::max(1u, renderHeight);
    if (config_.renderWidth == renderWidth && config_.renderHeight == renderHeight) {
        return;
    }
    config_.renderWidth = renderWidth;
    config_.renderHeight = renderHeight;
    backend_.SetRenderResolution(renderWidth, renderHeight);
}

void VulkanRenderer::BeginFrame() {
    if (!backend_.BeginFrame()) {
        backend_.RecreateSwapchain();
        if (!backend_.BeginFrame()) {
            throw std::runtime_error("VulkanRenderer BeginFrame failed after recreating an out-of-date swapchain");
        }
    }
}

void VulkanRenderer::BeginLogicalRender() {
    backend_.BeginLogicalRender();
}

void VulkanRenderer::EndLogicalRender() {
    backend_.EndLogicalRender();
}

void VulkanRenderer::Present() {
    backend_.Present();
}

void VulkanRenderer::EndFrame() {
    backend_.EndFrame();
    if (!backend_.PresentFrame()) {
        backend_.RecreateSwapchain();
    }
}

render::TextureHandle VulkanRenderer::LoadTexture(const std::filesystem::path& path,
                                                  render::TextureFilter filter,
                                                  render::TextureSizing sizing) {
    const render::TextureHandle handle = backend_.LoadRendererTexture(path, filter, sizing);
    if (handle.id != 0) {
        textureHandles_.insert(handle.id);
    }
    return handle;
}

render::TextureHandle VulkanRenderer::CreateTextureRGBA(const render::TextureDesc& desc, const void* pixels) {
    const render::TextureHandle handle = backend_.CreateRendererTextureRGBA(desc, pixels);
    if (handle.id != 0) {
        textureHandles_.insert(handle.id);
    }
    return handle;
}

void VulkanRenderer::DestroyTexture(render::TextureHandle texture) {
    backend_.WaitIdle();
    backend_.DestroyRendererTexture(texture);
    textureHandles_.erase(texture.id);
    targetTextureOwners_.erase(texture.id);
}

void VulkanRenderer::SetTextureFilter(render::TextureHandle texture, render::TextureFilter filter) {
    backend_.SetRendererTextureFilter(texture, filter);
}

render::Vec2 VulkanRenderer::TextureSize(render::TextureHandle texture) {
    return backend_.RendererTextureSize(texture);
}

render::RenderTargetHandle VulkanRenderer::CreateRenderTarget(std::uint32_t width, std::uint32_t height) {
    const render::RenderTargetHandle handle = backend_.CreateRendererRenderTarget(width, height);
    if (handle.id != 0) {
        targetHandles_.insert(handle.id);
    }
    return handle;
}

void VulkanRenderer::DestroyRenderTarget(render::RenderTargetHandle target) {
    backend_.WaitIdle();
    backend_.DestroyRendererRenderTarget(target);
    targetHandles_.erase(target.id);
    for (auto it = targetTextureOwners_.begin(); it != targetTextureOwners_.end();) {
        if (it->second == target.id) {
            textureHandles_.erase(it->first);
            it = targetTextureOwners_.erase(it);
        } else {
            ++it;
        }
    }
}

void VulkanRenderer::BeginRenderTarget(render::RenderTargetHandle target) {
    backend_.BeginRendererRenderTarget(target);
}

void VulkanRenderer::EndRenderTarget() {
    backend_.EndRendererRenderTarget();
}

render::TextureHandle VulkanRenderer::RenderTargetTexture(render::RenderTargetHandle target) {
    const render::TextureHandle handle = backend_.RendererRenderTargetTexture(target);
    if (handle.id != 0) {
        textureHandles_.insert(handle.id);
        targetTextureOwners_[handle.id] = target.id;
    }
    return handle;
}

std::vector<std::uint8_t> VulkanRenderer::CaptureRenderTargetRGBA(render::RenderTargetHandle target) {
    return backend_.CaptureRendererRenderTargetRGBA(target);
}

bool VulkanRenderer::UpscaleRenderTargetFsr1(render::RenderTargetHandle source,
                                             render::RenderTargetHandle destination,
                                             float sharpness) {
    return backend_.UpscaleRendererRenderTargetFsr1(source, destination, sharpness);
}

render::FontHandle VulkanRenderer::LoadFont(const std::filesystem::path& path, int size) {
    const render::FontHandle handle = backend_.LoadRendererFont(path, size);
    if (handle.id != 0) {
        fontHandles_.insert(handle.id);
    }
    return handle;
}

void VulkanRenderer::DestroyFont(render::FontHandle font) {
    backend_.WaitIdle();
    backend_.DestroyRendererFont(font);
    fontHandles_.erase(font.id);
}

render::Vec2 VulkanRenderer::MeasureText(render::FontHandle font, std::string_view text, float size) {
    return backend_.MeasureRendererText(font, text, size);
}

void VulkanRenderer::DrawTexture(const render::DrawTextureParams& params) {
    backend_.DrawRendererTexture(params);
}

void VulkanRenderer::DrawTextureSource(const render::DrawTextureParams& params) {
    backend_.DrawRendererTexture(params);
}

void VulkanRenderer::DrawTextureQuad(const render::DrawTextureQuadParams& params) {
    backend_.DrawRendererTextureQuad(params);
}

void VulkanRenderer::DrawAtlasFrame(const render::AtlasFrameParams& params) {
    backend_.DrawRendererAtlasFrame(params);
}

void VulkanRenderer::DrawRectangle(render::Rect rect, render::Color color, float alpha) {
    backend_.DrawRendererRectangle(rect, color, alpha);
}

void VulkanRenderer::DrawRectangleOutline(render::Rect rect, float thickness, render::Color color, float alpha) {
    backend_.DrawRendererRectangleOutline(rect, thickness, color, alpha);
}

void VulkanRenderer::DrawLine(render::Vec2 a, render::Vec2 b, float thickness, render::Color color, float alpha) {
    backend_.DrawRendererLine(a, b, thickness, color, alpha);
}

void VulkanRenderer::DrawCircle(render::Vec2 center, float radius, render::Color color, float alpha) {
    backend_.DrawRendererCircle(center, radius, color, alpha);
}

void VulkanRenderer::DrawText(const render::TextParams& params) {
    backend_.DrawRendererText(params);
}

void VulkanRenderer::PushScissor(render::Rect logicalRect) {
    backend_.PushRendererScissor(logicalRect);
}

void VulkanRenderer::PopScissor() {
    backend_.PopRendererScissor();
}

void VulkanRenderer::ClearScissor() {
    backend_.ClearRendererScissor();
}

render::Vec2 VulkanRenderer::LogicalSize() const {
    return {static_cast<float>(config_.logicalWidth), static_cast<float>(config_.logicalHeight)};
}

render::Rect VulkanRenderer::FinalPresentationRect() const {
    const VkRect2D rect = backend_.FinalPresentationRect();
    return {
        static_cast<float>(rect.offset.x),
        static_cast<float>(rect.offset.y),
        static_cast<float>(rect.extent.width),
        static_cast<float>(rect.extent.height)
    };
}

void VulkanRenderer::SetFinalPresentationRect(render::Rect rect) {
    backend_.SetFinalPresentationRect(rect);
}

void VulkanRenderer::SetOutputFilter(render::TextureFilter filter) {
    config_.outputFilter = filter;
    backend_.SetOutputLinearFilter(config_.outputFilter == render::TextureFilter::Linear);
}

void VulkanRenderer::SetOutputColorMode(render::OutputColorMode mode) {
    config_.requestedOutputColorMode = mode;
    backend_.SetHdrMode(toVulkanHdrMode(mode));
}

void VulkanRenderer::SetPostProcessSettings(const render::PostProcessSettings& settings) {
    backend_.SetPostProcessSettings(settings);
}

void VulkanRenderer::SetMsaaSamples(std::uint32_t samples) {
    config_.msaaSamples = samples;
    backend_.SetMsaaSamples(samples);
}

render::RendererStats VulkanRenderer::Stats() const {
    const wbwwb::vulkan::VulkanBackendStats stats = backend_.Stats();
    return {
        config_.logicalWidth,
        config_.logicalHeight,
        stats.renderTargetExtent.width,
        stats.renderTargetExtent.height,
        stats.swapchainExtent.width,
        stats.swapchainExtent.height,
        stats.hdrSwapchainAvailable,
        stats.hdrActive,
        backend_.ActiveMsaaSamples(),
        stats.hdrSystemEnabled,
        stats.hdrPaperWhiteNits,
        stats.hdrPeakNits
    };
}

} // namespace wb
