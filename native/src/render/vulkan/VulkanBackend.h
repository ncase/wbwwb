#pragma once

// Native WBWWB C++ Port Vulkan renderer backend.

#include "VulkanCommandContext.h"
#include "VulkanDevice.h"
#include "VulkanFramebuffers.h"
#include "VulkanFont.h"
#include "VulkanHDR.h"
#include "VulkanInstance.h"
#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderTarget.h"
#include "VulkanRenderer2D.h"
#include "VulkanSurface.h"
#include "VulkanSwapchain.h"
#include "VulkanSync.h"
#include "VulkanTexture.h"
#include "VulkanTypes.h"
#include "render/Renderer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace wbwwb::vulkan {

class VulkanBackend final {
public:
    VulkanBackend() = default;
    ~VulkanBackend();

    VulkanBackend(const VulkanBackend&) = delete;
    VulkanBackend& operator=(const VulkanBackend&) = delete;

    void Initialize(const VulkanBackendConfig& config);
    void Shutdown();
    void WaitIdle() const;

    // Returns false when the swapchain is out of date and the caller should resize/recreate.
    [[nodiscard]] bool BeginFrame();
    void ClearFrame();
    void BeginLogicalRender();
    void EndLogicalRender();
    void Present();
    void EndFrame();
    [[nodiscard]] bool PresentFrame();
    void Resize(std::uint32_t width, std::uint32_t height);
    void SetRenderResolution(std::uint32_t width, std::uint32_t height);
    void RecreateSwapchain();
    void SetUiState(const VulkanUiRuntimeState& state);
    void SetFinalPresentationRect(wb::render::Rect rect);
    void SetPresentationOptions(bool vsync, bool outputLinearFilter);
    void SetVsync(bool enabled);
    void SetOutputLinearFilter(bool enabled);
    void SetHdrMode(VulkanHdrMode mode);
    void SetPostProcessSettings(const wb::render::PostProcessSettings& settings);
    void SetMsaaSamples(std::uint32_t samples);
    [[nodiscard]] std::uint32_t ActiveMsaaSamples() const noexcept;

    [[nodiscard]] wb::render::TextureHandle LoadRendererTexture(const std::filesystem::path& path,
                                                                 wb::render::TextureFilter filter,
                                                                 wb::render::TextureSizing sizing = wb::render::TextureSizing::Source);
    [[nodiscard]] wb::render::TextureHandle CreateRendererTextureRGBA(const wb::render::TextureDesc& desc,
                                                                       const void* pixels);
    void DestroyRendererTexture(wb::render::TextureHandle texture);
    void SetRendererTextureFilter(wb::render::TextureHandle texture, wb::render::TextureFilter filter);
    [[nodiscard]] wb::render::Vec2 RendererTextureSize(wb::render::TextureHandle texture) const;

    [[nodiscard]] wb::render::RenderTargetHandle CreateRendererRenderTarget(std::uint32_t width, std::uint32_t height);
    void DestroyRendererRenderTarget(wb::render::RenderTargetHandle target);
    void BeginRendererRenderTarget(wb::render::RenderTargetHandle target);
    void EndRendererRenderTarget();
    [[nodiscard]] wb::render::TextureHandle RendererRenderTargetTexture(wb::render::RenderTargetHandle target);
    [[nodiscard]] std::vector<std::uint8_t> CaptureRendererRenderTargetRGBA(wb::render::RenderTargetHandle target);
    [[nodiscard]] bool UpscaleRendererRenderTargetFsr1(wb::render::RenderTargetHandle source,
                                                       wb::render::RenderTargetHandle destination,
                                                       float sharpness);

    [[nodiscard]] wb::render::FontHandle LoadRendererFont(const std::filesystem::path& path, int size);
    void DestroyRendererFont(wb::render::FontHandle font);
    [[nodiscard]] wb::render::Vec2 MeasureRendererText(wb::render::FontHandle font,
                                                       std::string_view text,
                                                       float size) const;

    void DrawRendererTexture(const wb::render::DrawTextureParams& params);
    void DrawRendererTextureQuad(const wb::render::DrawTextureQuadParams& params);
    void DrawRendererAtlasFrame(const wb::render::AtlasFrameParams& params);
    void DrawRendererRectangle(wb::render::Rect rect, wb::render::Color color, float alpha = 1.0f);
    void DrawRendererRectangleOutline(wb::render::Rect rect, float thickness, wb::render::Color color, float alpha = 1.0f);
    void DrawRendererLine(wb::render::Vec2 a, wb::render::Vec2 b, float thickness, wb::render::Color color, float alpha = 1.0f);
    void DrawRendererCircle(wb::render::Vec2 center, float radius, wb::render::Color color, float alpha = 1.0f);
    void DrawRendererText(const wb::render::TextParams& params);
    void PushRendererScissor(wb::render::Rect logicalRect);
    void PopRendererScissor();
    void ClearRendererScissor();

    void SetClearColor(float r, float g, float b, float a) noexcept;

    [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }
    [[nodiscard]] VulkanBackendStats Stats() const noexcept;
    [[nodiscard]] VkRect2D FinalPresentationRect() const noexcept;

private:
    [[nodiscard]] std::vector<const char*> BuildRequiredInstanceExtensions(const VulkanBackendConfig& config) const;
    void CreateSurface(const VulkanBackendConfig& config);
    void CreateSwapchainResources();
    void DestroySwapchainResources();
    void ApplyHdrMetadata();
    [[nodiscard]] float EffectiveHdrPaperWhiteNits() const noexcept;
    [[nodiscard]] float EffectiveHdrPeakNits() const noexcept;
    [[nodiscard]] VkRect2D LetterboxRect() const noexcept;
    void RecordFrame(VkCommandBuffer commandBuffer, std::uint32_t imageIndex);
    void RecordLogicalScenePass(VkCommandBuffer commandBuffer);
    void RecordPresentPass(VkCommandBuffer commandBuffer, std::uint32_t imageIndex);
    void LoadUiResources();
    void DestroyUiResources();
    void CreateUiDescriptors();
    void LoadUiLocalization();
    void DrawSmokeScene();
    void DrawMenuTestScene();
    void DrawSettingsTestScene();
    void DrawSharedGameBoundaryScene();
    void DrawRectOutline(float x, float y, float width, float height, float thickness, VulkanRenderer2D::Color color);
    void DrawText(std::string_view key, float x, float y, float size, VulkanRenderer2D::Color color);
    void DrawTextRaw(std::string_view text, float x, float y, float size, VulkanRenderer2D::Color color);
    void DrawTextCentered(std::string_view text, float centerX, float centerY, float size, VulkanRenderer2D::Color color);
    void DrawTextCenteredFit(std::string_view text,
                             float centerX,
                             float centerY,
                             float size,
                             float maxWidth,
                             VulkanRenderer2D::Color color);
    [[nodiscard]] const std::string& UiString(std::string_view key) const;

    struct UiTexture {
        VulkanTexture texture;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
    };

    struct RendererTexture {
        VulkanTexture texture;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
        wb::render::TextureFilter filter = wb::render::TextureFilter::Linear;
    };

    struct RendererTarget {
        VulkanRenderTarget target;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
        wb::render::TextureFilter filter = wb::render::TextureFilter::Linear;
        bool hasContent = false;
    };

    struct RendererFont {
        VulkanFont font;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
    };

    [[nodiscard]] RendererTexture* FindRendererTexture(wb::render::TextureHandle texture);
    [[nodiscard]] const RendererTexture* FindRendererTexture(wb::render::TextureHandle texture) const;
    [[nodiscard]] RendererFont* FindRendererFont(wb::render::FontHandle font);
    [[nodiscard]] const RendererFont* FindRendererFont(wb::render::FontHandle font) const;
    [[nodiscard]] RendererTarget* FindRendererTarget(wb::render::RenderTargetHandle target);
    [[nodiscard]] const RendererTarget* FindRendererTarget(wb::render::RenderTargetHandle target) const;
    void WarnUnsupportedOnce(const char* key, const char* message) const;
    void EnsureExternalDrawOpen(const char* action) const;

    VulkanInstance instance_;
    VulkanSurface surface_;
    VulkanDevice device_;
    VulkanSwapchain swapchain_;
    VulkanRenderPass sceneRenderPass_;
    VulkanRenderPass swapchainRenderPass_;
    VulkanRenderTarget logicalTarget_;
    VulkanFramebuffers framebuffers_;
    VulkanCommandContext commandContext_;
    VulkanSync sync_;
    VulkanPipeline scenePipeline_;
    VulkanPipeline presentPipeline_;
    VulkanPipeline fsrEasuPipeline_;
    VulkanPipeline fsrRcasPipeline_;
    VulkanRenderer2D sceneRenderer2D_;
    VulkanRenderer2D presentRenderer2D_;
    VulkanRenderer2D fsrEasuRenderer2D_;
    VulkanRenderer2D fsrRcasRenderer2D_;
    VulkanRenderTarget fsrIntermediateTarget_;
    VulkanTexture smokeTexture_;
    VulkanFont uiFont_;
    UiTexture uiBgPreload_;
    UiTexture uiBgPreload2_;
    UiTexture uiBg_;
    UiTexture uiTv_;
    VkDescriptorSet smokeTextureDescriptor_ = VK_NULL_HANDLE;
    VkDescriptorSet logicalTargetDescriptor_ = VK_NULL_HANDLE;
    VkDescriptorSet uiFontDescriptor_ = VK_NULL_HANDLE;
    VulkanHDR hdrProbe_;
    VulkanDisplayHDRState displayHdrState_{};
    std::unordered_map<std::string, std::string> uiStrings_;
    VulkanUiRuntimeState uiState_;
    std::unordered_map<std::uint32_t, std::unique_ptr<RendererTexture>> rendererTextures_;
    std::unordered_map<std::uint32_t, std::unique_ptr<RendererFont>> rendererFonts_;
    std::unordered_map<std::uint32_t, std::unique_ptr<RendererTarget>> rendererTargets_;
    std::unordered_map<std::uint32_t, std::uint32_t> rendererTargetTextureOwners_;
    std::vector<wb::render::Rect> rendererScissorStack_;
    mutable std::unordered_map<std::string, bool> unsupportedWarnings_;
    wb::render::RenderTargetHandle activeRendererTarget_{};
    VkCommandBuffer activeRendererTargetCommandBuffer_ = VK_NULL_HANDLE;

    VulkanBackendConfig config_;
    std::uint32_t currentFrame_ = 0;
    std::uint32_t currentImageIndex_ = 0;
    std::uint32_t nextRendererTextureId_ = 1;
    std::uint32_t nextRendererFontId_ = 1;
    std::uint32_t nextRendererTargetId_ = 1;
    bool initialized_ = false;
    bool frameOpen_ = false;
    bool clearRecorded_ = false;
    bool logicalPassOpen_ = false;
    bool logicalPassRecorded_ = false;
    bool presentPassRecorded_ = false;
    bool rendererTargetPassOpen_ = false;
    bool debugDeviceLogged_ = false;
    bool debugSwapchainLogged_ = false;
    bool hdrMetadataDirty_ = true;
    std::optional<VkRect2D> finalPresentationRect_;
    wb::render::PostProcessSettings postProcess_{};
    VkSampleCountFlagBits activeSampleCount_ = VK_SAMPLE_COUNT_1_BIT;
};

} // namespace wbwwb::vulkan
