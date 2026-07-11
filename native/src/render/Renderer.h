#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#ifdef DrawText
#undef DrawText
#endif

namespace wb::render {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

enum class TextureFilter {
    Nearest,
    Linear
};

enum class TextureSizing {
    Source,
    RenderResolution
};

enum class OutputColorMode {
    Sdr,
    HdrAuto,
    Hdr
};

struct PostProcessSettings {
    bool fxaa = false;
    float crtStrength = 0.0f;
    float noiseAmount = 0.0f;
    float brightness = 1.0f;
    float contrast = 1.0f;
    float sharpness = 1.0f;
    float gamma = 1.0f;
    float blackLevel = 0.0f;
    float whiteLevel = 1.0f;
    float screenBorder = 0.0f;
    float scanlines = 0.0f;
    float crtCurve = 0.0f;
    float hdrPaperWhiteNits = 203.0f;
    float hdrPeakNits = 1000.0f;
    float hdrHighlightStrength = 0.35f;
    float time = 0.0f;
};

struct TextureHandle {
    std::uint32_t id = 0;
};

struct RenderTargetHandle {
    std::uint32_t id = 0;
};

struct FontHandle {
    std::uint32_t id = 0;
};

struct SoundHandle {
    std::uint32_t id = 0;
};

struct MusicHandle {
    std::uint32_t id = 0;
};

struct TextureDesc {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    TextureFilter filter = TextureFilter::Linear;
};

struct DrawTextureParams {
    TextureHandle texture;
    Rect source;
    Rect destination;
    Vec2 origin;
    float rotationRadians = 0.0f;
    Color tint{};
    float alpha = 1.0f;
    bool flipX = false;
    bool flipY = false;
};

struct DrawTextureQuadParams {
    TextureHandle texture;
    Rect source;
    std::array<Vec2, 4> vertices;
    Color tint{};
    float alpha = 1.0f;
    bool flipX = false;
    bool flipY = false;
};

struct TextParams {
    FontHandle font;
    std::string_view text;
    Vec2 position;
    float size = 16.0f;
    Color color{};
    float alpha = 1.0f;
};

struct AtlasFrameParams {
    TextureHandle texture;
    Rect frame;
    Vec2 logicalFrameSize;
    Rect spriteSourceSize;
    Vec2 sourceSize;
    Vec2 position;
    Vec2 anchor;
    Vec2 scale{1.0f, 1.0f};
    float rotationRadians = 0.0f;
    Color tint{};
    float alpha = 1.0f;
    bool rotated = false;
};

struct RendererConfig {
    std::uint32_t logicalWidth = 960;
    std::uint32_t logicalHeight = 540;
    std::uint32_t renderWidth = 960;
    std::uint32_t renderHeight = 540;
    std::uint32_t outputWidth = 960;
    std::uint32_t outputHeight = 540;
    bool vsync = true;
    TextureFilter outputFilter = TextureFilter::Linear;
    OutputColorMode requestedOutputColorMode = OutputColorMode::Sdr;
    std::uint32_t msaaSamples = 1;
};

struct RendererStats {
    std::uint32_t logicalWidth = 960;
    std::uint32_t logicalHeight = 540;
    std::uint32_t renderWidth = 960;
    std::uint32_t renderHeight = 540;
    std::uint32_t outputWidth = 960;
    std::uint32_t outputHeight = 540;
    bool hdrAvailable = false;
    bool hdrActive = false;
    std::uint32_t msaaSamples = 1;
    bool hdrSystemEnabled = false;
    float hdrPaperWhiteNits = 203.0f;
    float hdrPeakNits = 1000.0f;
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void Initialize(const RendererConfig& config) = 0;
    virtual void Shutdown() = 0;
    virtual void Resize(std::uint32_t outputWidth, std::uint32_t outputHeight) = 0;
    virtual void SetRenderResolution(std::uint32_t renderWidth, std::uint32_t renderHeight) = 0;

    virtual void BeginFrame() = 0;
    virtual void BeginLogicalRender() = 0;
    virtual void EndLogicalRender() = 0;
    virtual void Present() = 0;
    virtual void EndFrame() = 0;

    virtual TextureHandle LoadTexture(const std::filesystem::path& path,
                                      TextureFilter filter,
                                      TextureSizing sizing = TextureSizing::Source) = 0;
    virtual TextureHandle CreateTextureRGBA(const TextureDesc& desc, const void* pixels) = 0;
    virtual void DestroyTexture(TextureHandle texture) = 0;
    virtual void SetTextureFilter(TextureHandle texture, TextureFilter filter) = 0;
    virtual Vec2 TextureSize(TextureHandle texture) = 0;

    virtual RenderTargetHandle CreateRenderTarget(std::uint32_t width, std::uint32_t height) = 0;
    virtual void DestroyRenderTarget(RenderTargetHandle target) = 0;
    virtual void BeginRenderTarget(RenderTargetHandle target) = 0;
    virtual void EndRenderTarget() = 0;
    virtual TextureHandle RenderTargetTexture(RenderTargetHandle target) = 0;
    virtual std::vector<std::uint8_t> CaptureRenderTargetRGBA(RenderTargetHandle target) = 0;
    virtual bool SupportsRenderTargetCapture() const = 0;
    virtual bool SupportsFsr1() const { return false; }
    virtual bool UpscaleRenderTargetFsr1(RenderTargetHandle source,
                                         RenderTargetHandle destination,
                                         float sharpness) {
        (void)source;
        (void)destination;
        (void)sharpness;
        return false;
    }

    virtual FontHandle LoadFont(const std::filesystem::path& path, int size) = 0;
    virtual void DestroyFont(FontHandle font) = 0;
    virtual Vec2 MeasureText(FontHandle font, std::string_view text, float size) = 0;

    virtual void DrawTexture(const DrawTextureParams& params) = 0;
    virtual void DrawTextureSource(const DrawTextureParams& params) = 0;
    virtual void DrawTextureQuad(const DrawTextureQuadParams& params) = 0;
    virtual void DrawAtlasFrame(const AtlasFrameParams& params) = 0;
    virtual void DrawRectangle(Rect rect, Color color, float alpha = 1.0f) = 0;
    virtual void DrawRectangleOutline(Rect rect, float thickness, Color color, float alpha = 1.0f) = 0;
    virtual void DrawLine(Vec2 a, Vec2 b, float thickness, Color color, float alpha = 1.0f) = 0;
    virtual void DrawCircle(Vec2 center, float radius, Color color, float alpha = 1.0f) = 0;
    virtual void DrawText(const TextParams& params) = 0;

    virtual void PushScissor(Rect logicalRect) = 0;
    virtual void PopScissor() = 0;
    virtual void ClearScissor() = 0;

    virtual Vec2 LogicalSize() const = 0;
    virtual Rect FinalPresentationRect() const = 0;
    virtual void SetFinalPresentationRect(Rect rect) { (void)rect; }
    virtual void SetOutputFilter(TextureFilter filter) = 0;
    virtual void SetOutputColorMode(OutputColorMode mode) { (void)mode; }
    virtual void SetPostProcessSettings(const PostProcessSettings& settings) { (void)settings; }
    virtual void SetMsaaSamples(std::uint32_t samples) { (void)samples; }

    virtual RendererStats Stats() const = 0;
};

} // namespace wb::render
