#pragma once

#include "render/Renderer.h"

#include "raylib.h"

#include <unordered_map>
#include <vector>

namespace wb {

class RaylibRenderer final : public render::Renderer {
public:
    RaylibRenderer() = default;
    ~RaylibRenderer() override;

    void Initialize(const render::RendererConfig& config) override;
    void Shutdown() override;
    void Resize(std::uint32_t outputWidth, std::uint32_t outputHeight) override;
    void SetRenderResolution(std::uint32_t renderWidth, std::uint32_t renderHeight) override;

    void BeginFrame() override;
    void BeginLogicalRender() override;
    void EndLogicalRender() override;
    void Present() override;
    void EndFrame() override;

    render::TextureHandle LoadTexture(const std::filesystem::path& path,
                                      render::TextureFilter filter,
                                      render::TextureSizing sizing = render::TextureSizing::Source) override;
    render::TextureHandle CreateTextureRGBA(const render::TextureDesc& desc, const void* pixels) override;
    void DestroyTexture(render::TextureHandle texture) override;
    void SetTextureFilter(render::TextureHandle texture, render::TextureFilter filter) override;
    render::Vec2 TextureSize(render::TextureHandle texture) override;

    render::RenderTargetHandle CreateRenderTarget(std::uint32_t width, std::uint32_t height) override;
    void DestroyRenderTarget(render::RenderTargetHandle target) override;
    void BeginRenderTarget(render::RenderTargetHandle target) override;
    void EndRenderTarget() override;
    render::TextureHandle RenderTargetTexture(render::RenderTargetHandle target) override;
    std::vector<std::uint8_t> CaptureRenderTargetRGBA(render::RenderTargetHandle target) override;
    bool SupportsRenderTargetCapture() const override { return true; }

    render::FontHandle LoadFont(const std::filesystem::path& path, int size) override;
    void DestroyFont(render::FontHandle font) override;
    render::Vec2 MeasureText(render::FontHandle font, std::string_view text, float size) override;

    void DrawTexture(const render::DrawTextureParams& params) override;
    void DrawTextureSource(const render::DrawTextureParams& params) override;
    void DrawTextureQuad(const render::DrawTextureQuadParams& params) override;
    void DrawAtlasFrame(const render::AtlasFrameParams& params) override;
    void DrawRectangle(render::Rect rect, render::Color color, float alpha = 1.0f) override;
    void DrawRectangleOutline(render::Rect rect, float thickness, render::Color color, float alpha = 1.0f) override;
    void DrawLine(render::Vec2 a, render::Vec2 b, float thickness, render::Color color, float alpha = 1.0f) override;
    void DrawCircle(render::Vec2 center, float radius, render::Color color, float alpha = 1.0f) override;
    void DrawText(const render::TextParams& params) override;

    void PushScissor(render::Rect logicalRect) override;
    void PopScissor() override;
    void ClearScissor() override;

    render::Vec2 LogicalSize() const override;
    render::Rect FinalPresentationRect() const override;
    void SetOutputFilter(render::TextureFilter filter) override;

    render::RendererStats Stats() const override;

    void BeginExternalLogicalRender();
    void EndExternalLogicalRender();

    Texture2D* raylibTexture(render::TextureHandle handle);
    RenderTexture2D* raylibRenderTarget(render::RenderTargetHandle handle);

private:
    struct FontEntry {
        Font font{};
        bool ownsFont = true;
    };

    Texture2D* findTexture(render::TextureHandle handle);
    Font* findFont(render::FontHandle handle);
    void unloadLogicalTarget();
    void ensureLogicalTarget();

    render::RendererConfig config_{};
    RenderTexture2D logicalTarget_{};
    std::unordered_map<std::uint32_t, Texture2D> textures_;
    std::unordered_map<std::uint32_t, RenderTexture2D> targets_;
    std::unordered_map<std::uint32_t, FontEntry> fonts_;
    std::unordered_map<std::uint32_t, std::uint32_t> targetTextureOwners_;
    std::vector<render::Rect> scissorStack_;
    std::uint32_t nextTextureId_ = 1;
    std::uint32_t nextTargetId_ = 1;
    std::uint32_t nextFontId_ = 1;
    std::uint32_t logicalTargetTextureId_ = 0;
    bool frameBegun_ = false;
    bool logicalBegun_ = false;
    bool externalLogicalBegun_ = false;
};

}  // namespace wb
