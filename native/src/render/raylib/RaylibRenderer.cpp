#include "render/raylib/RaylibRenderer.h"

#include "rlgl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

namespace wb {
namespace {

Color toRaylib(render::Color color, float alpha = 1.0f) {
    color.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * static_cast<float>(color.a));
    return Color{color.r, color.g, color.b, color.a};
}

Rectangle toRaylib(render::Rect rect) {
    return Rectangle{rect.x, rect.y, rect.width, rect.height};
}

Vector2 toRaylib(render::Vec2 value) {
    return Vector2{value.x, value.y};
}

int toRaylibFilter(render::TextureFilter filter) {
    return filter == render::TextureFilter::Nearest ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR;
}

render::Rect targetRect(Texture2D texture) {
    return {0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)};
}

render::Rect mapLogicalScissor(render::Rect rect, const render::RendererConfig& config, bool logicalTargetActive) {
    const float logicalWidth = static_cast<float>(std::max<std::uint32_t>(1, config.logicalWidth));
    const float logicalHeight = static_cast<float>(std::max<std::uint32_t>(1, config.logicalHeight));
    const std::uint32_t targetWidth = logicalTargetActive ? config.renderWidth : config.outputWidth;
    const std::uint32_t targetHeight = logicalTargetActive ? config.renderHeight : config.outputHeight;
    const float scaleX = static_cast<float>(std::max<std::uint32_t>(1, targetWidth)) / logicalWidth;
    const float scaleY = static_cast<float>(std::max<std::uint32_t>(1, targetHeight)) / logicalHeight;
    return {rect.x * scaleX, rect.y * scaleY, rect.width * scaleX, rect.height * scaleY};
}

std::vector<int> latinUiCodepoints() {
    std::vector<int> codepoints;
    codepoints.reserve(352);
    for (int codepoint = 32; codepoint <= 126; ++codepoint) {
        codepoints.push_back(codepoint);
    }
    for (int codepoint = 160; codepoint <= 383; ++codepoint) {
        codepoints.push_back(codepoint);
    }
    return codepoints;
}

}  // namespace

RaylibRenderer::~RaylibRenderer() {
    Shutdown();
}

void RaylibRenderer::Initialize(const render::RendererConfig& config) {
    config_ = config;
}

void RaylibRenderer::Shutdown() {
    ClearScissor();
    unloadLogicalTarget();
    for (auto& [_, target] : targets_) {
        if (target.id != 0) {
            UnloadRenderTexture(target);
        }
    }
    targets_.clear();
    targetTextureOwners_.clear();
    for (auto& [_, texture] : textures_) {
        if (texture.id != 0) {
            UnloadTexture(texture);
        }
    }
    textures_.clear();
    for (auto& [_, entry] : fonts_) {
        if (entry.ownsFont && entry.font.texture.id != 0) {
            UnloadFont(entry.font);
        }
    }
    fonts_.clear();
}

void RaylibRenderer::Resize(std::uint32_t outputWidth, std::uint32_t outputHeight) {
    config_.outputWidth = outputWidth;
    config_.outputHeight = outputHeight;
}

void RaylibRenderer::SetRenderResolution(std::uint32_t renderWidth, std::uint32_t renderHeight) {
    renderWidth = std::max(1u, renderWidth);
    renderHeight = std::max(1u, renderHeight);
    if (config_.renderWidth == renderWidth && config_.renderHeight == renderHeight) {
        return;
    }
    config_.renderWidth = renderWidth;
    config_.renderHeight = renderHeight;
    if (logicalTarget_.id != 0) {
        ensureLogicalTarget();
    }
}

void RaylibRenderer::BeginFrame() {
    BeginDrawing();
    frameBegun_ = true;
}

void RaylibRenderer::BeginLogicalRender() {
    ensureLogicalTarget();
    BeginTextureMode(logicalTarget_);
    ClearBackground(BLACK);
    rlPushMatrix();
    rlScalef(
        static_cast<float>(std::max(1u, config_.renderWidth)) / static_cast<float>(std::max(1u, config_.logicalWidth)),
        static_cast<float>(std::max(1u, config_.renderHeight)) / static_cast<float>(std::max(1u, config_.logicalHeight)),
        1.0f
    );
    logicalBegun_ = true;
}

void RaylibRenderer::EndLogicalRender() {
    if (logicalBegun_) {
        ClearScissor();
        rlPopMatrix();
        EndTextureMode();
        logicalBegun_ = false;
    }
}

void RaylibRenderer::BeginExternalLogicalRender() {
    externalLogicalBegun_ = true;
}

void RaylibRenderer::EndExternalLogicalRender() {
    ClearScissor();
    externalLogicalBegun_ = false;
}

void RaylibRenderer::Present() {
    if (logicalTarget_.id == 0 || logicalTarget_.texture.id == 0) {
        return;
    }
    ClearBackground(BLACK);
    const Rectangle source{0.0f, 0.0f, static_cast<float>(logicalTarget_.texture.width), -static_cast<float>(logicalTarget_.texture.height)};
    DrawTexturePro(logicalTarget_.texture, source, toRaylib(FinalPresentationRect()), {0.0f, 0.0f}, 0.0f, WHITE);
}

void RaylibRenderer::EndFrame() {
    if (frameBegun_) {
        EndDrawing();
        frameBegun_ = false;
    }
}

render::TextureHandle RaylibRenderer::LoadTexture(const std::filesystem::path& path,
                                                  render::TextureFilter filter,
                                                  render::TextureSizing sizing) {
    Texture2D loaded{};
    if (sizing == render::TextureSizing::RenderResolution) {
        Image image = LoadImage(path.string().c_str());
        if (image.data != nullptr) {
            const int width = static_cast<int>(std::max(1u, config_.renderWidth));
            const int height = static_cast<int>(std::max(1u, config_.renderHeight));
            if (image.width != width || image.height != height) {
                if (filter == render::TextureFilter::Nearest) {
                    ImageResizeNN(&image, width, height);
                } else {
                    ImageResize(&image, width, height);
                }
            }
            loaded = LoadTextureFromImage(image);
            UnloadImage(image);
        }
    } else {
        loaded = ::LoadTexture(path.string().c_str());
    }
    if (loaded.id == 0) {
        std::cerr << "Warning: failed to load raylib texture " << path << "\n";
        return {};
    }
    ::SetTextureFilter(loaded, toRaylibFilter(filter));
    const render::TextureHandle handle{nextTextureId_++};
    textures_[handle.id] = loaded;
    return handle;
}

render::TextureHandle RaylibRenderer::CreateTextureRGBA(const render::TextureDesc& desc, const void* pixels) {
    if (!pixels || desc.width == 0 || desc.height == 0) {
        return {};
    }
    Image image{};
    image.data = const_cast<void*>(pixels);
    image.width = static_cast<int>(desc.width);
    image.height = static_cast<int>(desc.height);
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    Texture2D loaded = LoadTextureFromImage(image);
    if (loaded.id == 0) {
        return {};
    }
    ::SetTextureFilter(loaded, toRaylibFilter(desc.filter));
    const render::TextureHandle handle{nextTextureId_++};
    textures_[handle.id] = loaded;
    return handle;
}

void RaylibRenderer::DestroyTexture(render::TextureHandle texture) {
    auto it = textures_.find(texture.id);
    if (it == textures_.end()) {
        return;
    }
    if (it->second.id != 0) {
        UnloadTexture(it->second);
    }
    textures_.erase(it);
}

void RaylibRenderer::SetTextureFilter(render::TextureHandle texture, render::TextureFilter filter) {
    if (Texture2D* value = findTexture(texture)) {
        ::SetTextureFilter(*value, toRaylibFilter(filter));
    }
}

render::Vec2 RaylibRenderer::TextureSize(render::TextureHandle texture) {
    if (Texture2D* value = findTexture(texture)) {
        return {static_cast<float>(value->width), static_cast<float>(value->height)};
    }
    return {};
}

render::RenderTargetHandle RaylibRenderer::CreateRenderTarget(std::uint32_t width, std::uint32_t height) {
    RenderTexture2D target = LoadRenderTexture(static_cast<int>(width), static_cast<int>(height));
    if (target.id == 0) {
        return {};
    }
    ::SetTextureFilter(target.texture, toRaylibFilter(config_.outputFilter));
    const render::RenderTargetHandle handle{nextTargetId_++};
    targets_[handle.id] = target;
    return handle;
}

void RaylibRenderer::DestroyRenderTarget(render::RenderTargetHandle target) {
    auto it = targets_.find(target.id);
    if (it == targets_.end()) {
        return;
    }
    for (auto alias = targetTextureOwners_.begin(); alias != targetTextureOwners_.end();) {
        if (alias->second == target.id) {
            alias = targetTextureOwners_.erase(alias);
        } else {
            ++alias;
        }
    }
    if (it->second.id != 0) {
        UnloadRenderTexture(it->second);
    }
    targets_.erase(it);
}

void RaylibRenderer::BeginRenderTarget(render::RenderTargetHandle target) {
    if (RenderTexture2D* value = raylibRenderTarget(target)) {
        BeginTextureMode(*value);
    }
}

void RaylibRenderer::EndRenderTarget() {
    EndTextureMode();
}

render::TextureHandle RaylibRenderer::RenderTargetTexture(render::RenderTargetHandle target) {
    if (!raylibRenderTarget(target)) {
        return {};
    }
    for (const auto& [textureId, ownerId] : targetTextureOwners_) {
        if (ownerId == target.id) {
            return render::TextureHandle{textureId};
        }
    }
    const render::TextureHandle handle{nextTextureId_++};
    targetTextureOwners_[handle.id] = target.id;
    return handle;
}

std::vector<std::uint8_t> RaylibRenderer::CaptureRenderTargetRGBA(render::RenderTargetHandle target) {
    RenderTexture2D* value = raylibRenderTarget(target);
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

render::FontHandle RaylibRenderer::LoadFont(const std::filesystem::path& path, int size) {
    FontEntry entry{};
    if (!path.empty() && std::filesystem::exists(path)) {
        std::vector<int> glyphs = latinUiCodepoints();
        entry.font = LoadFontEx(path.string().c_str(), size, glyphs.data(), static_cast<int>(glyphs.size()));
    }
    if (entry.font.texture.id == 0) {
        entry.font = GetFontDefault();
        entry.ownsFont = false;
    }
    const render::FontHandle handle{nextFontId_++};
    fonts_[handle.id] = entry;
    return handle;
}

void RaylibRenderer::DestroyFont(render::FontHandle font) {
    auto it = fonts_.find(font.id);
    if (it == fonts_.end()) {
        return;
    }
    if (it->second.ownsFont && it->second.font.texture.id != 0) {
        UnloadFont(it->second.font);
    }
    fonts_.erase(it);
}

render::Vec2 RaylibRenderer::MeasureText(render::FontHandle font, std::string_view text, float size) {
    Font* value = findFont(font);
    if (!value) {
        return {};
    }
    const std::string copy(text);
    const Vector2 measured = MeasureTextEx(*value, copy.c_str(), size, 0.0f);
    return {measured.x, measured.y};
}

void RaylibRenderer::DrawTexture(const render::DrawTextureParams& params) {
    DrawTextureSource(params);
}

void RaylibRenderer::DrawTextureSource(const render::DrawTextureParams& params) {
    Texture2D* texture = findTexture(params.texture);
    if (!texture || texture->id == 0) {
        return;
    }
    Rectangle source = toRaylib(params.source.width == 0.0f && params.source.height == 0.0f ? targetRect(*texture) : params.source);
    if (params.flipX) {
        source.x += source.width;
        source.width *= -1.0f;
    }
    if (params.flipY) {
        source.y += source.height;
        source.height *= -1.0f;
    }
    DrawTexturePro(
        *texture,
        source,
        toRaylib(params.destination),
        toRaylib(params.origin),
        params.rotationRadians * RAD2DEG,
        toRaylib(params.tint, params.alpha)
    );
}

void RaylibRenderer::DrawTextureQuad(const render::DrawTextureQuadParams& params) {
    Texture2D* texture = findTexture(params.texture);
    if (!texture || texture->id == 0) {
        return;
    }

    const render::Rect source = params.source.width == 0.0f && params.source.height == 0.0f ? targetRect(*texture) : params.source;
    const float width = static_cast<float>(texture->width);
    const float height = static_cast<float>(texture->height);
    if (width <= 0.0f || height <= 0.0f || source.width == 0.0f || source.height == 0.0f) {
        return;
    }

    const float sourceLeft = source.x / width;
    const float sourceTop = source.y / height;
    const float sourceRight = (source.x + source.width) / width;
    const float sourceBottom = (source.y + source.height) / height;
    const float left = params.flipX ? sourceRight : sourceLeft;
    const float right = params.flipX ? sourceLeft : sourceRight;
    const float top = params.flipY ? sourceBottom : sourceTop;
    const float bottom = params.flipY ? sourceTop : sourceBottom;
    const Color color = toRaylib(params.tint, params.alpha);

    rlSetTexture(texture->id);
    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlNormal3f(0.0f, 0.0f, 1.0f);

        rlTexCoord2f(left, top);
        rlVertex2f(params.vertices[0].x, params.vertices[0].y);

        rlTexCoord2f(left, bottom);
        rlVertex2f(params.vertices[1].x, params.vertices[1].y);

        rlTexCoord2f(right, bottom);
        rlVertex2f(params.vertices[2].x, params.vertices[2].y);

        rlTexCoord2f(right, top);
        rlVertex2f(params.vertices[3].x, params.vertices[3].y);
    rlEnd();
    rlSetTexture(0);
}

void RaylibRenderer::DrawAtlasFrame(const render::AtlasFrameParams& params) {
    if (params.rotated) {
        static bool warned = false;
        if (!warned) {
            std::cerr << "Warning: rotated atlas frames are not yet supported by RaylibRenderer abstraction\n";
            warned = true;
        }
        return;
    }

    const float absScaleX = std::abs(params.scale.x);
    const float absScaleY = std::abs(params.scale.y);
    render::Rect source = params.frame;
    if (params.scale.x < 0.0f) {
        source.x += source.width;
        source.width *= -1.0f;
    }
    if (params.scale.y < 0.0f) {
        source.y += source.height;
        source.height *= -1.0f;
    }

    render::DrawTextureParams draw{};
    draw.texture = params.texture;
    draw.source = source;
    draw.destination = {
        params.position.x,
        params.position.y,
        params.logicalFrameSize.x * absScaleX,
        params.logicalFrameSize.y * absScaleY
    };
    draw.origin = {
        (params.anchor.x * params.sourceSize.x - params.spriteSourceSize.x) * absScaleX,
        (params.anchor.y * params.sourceSize.y - params.spriteSourceSize.y) * absScaleY
    };
    if (params.scale.x < 0.0f) {
        draw.origin.x = draw.destination.width - draw.origin.x;
    }
    if (params.scale.y < 0.0f) {
        draw.origin.y = draw.destination.height - draw.origin.y;
    }
    draw.rotationRadians = params.rotationRadians;
    draw.tint = params.tint;
    draw.alpha = params.alpha;
    DrawTextureSource(draw);
}

void RaylibRenderer::DrawRectangle(render::Rect rect, render::Color color, float alpha) {
    DrawRectangleRec(toRaylib(rect), toRaylib(color, alpha));
}

void RaylibRenderer::DrawRectangleOutline(render::Rect rect, float thickness, render::Color color, float alpha) {
    DrawRectangleLinesEx(toRaylib(rect), thickness, toRaylib(color, alpha));
}

void RaylibRenderer::DrawLine(render::Vec2 a, render::Vec2 b, float thickness, render::Color color, float alpha) {
    DrawLineEx(toRaylib(a), toRaylib(b), thickness, toRaylib(color, alpha));
}

void RaylibRenderer::DrawCircle(render::Vec2 center, float radius, render::Color color, float alpha) {
    DrawCircleV(toRaylib(center), radius, toRaylib(color, alpha));
}

void RaylibRenderer::DrawText(const render::TextParams& params) {
    Font* font = findFont(params.font);
    if (!font) {
        return;
    }
    const std::string copy(params.text);
    DrawTextEx(*font, copy.c_str(), toRaylib(params.position), params.size, 0.0f, toRaylib(params.color, params.alpha));
}

void RaylibRenderer::PushScissor(render::Rect logicalRect) {
    scissorStack_.push_back(logicalRect);
    const render::Rect renderRect = mapLogicalScissor(logicalRect, config_, logicalBegun_ || externalLogicalBegun_);
    BeginScissorMode(
        static_cast<int>(std::floor(renderRect.x)),
        static_cast<int>(std::floor(renderRect.y)),
        static_cast<int>(std::ceil(renderRect.width)),
        static_cast<int>(std::ceil(renderRect.height))
    );
}

void RaylibRenderer::PopScissor() {
    if (scissorStack_.empty()) {
        EndScissorMode();
        return;
    }
    scissorStack_.pop_back();
    EndScissorMode();
    if (!scissorStack_.empty()) {
        const render::Rect rect = mapLogicalScissor(scissorStack_.back(), config_, logicalBegun_ || externalLogicalBegun_);
        BeginScissorMode(
            static_cast<int>(std::floor(rect.x)),
            static_cast<int>(std::floor(rect.y)),
            static_cast<int>(std::ceil(rect.width)),
            static_cast<int>(std::ceil(rect.height))
        );
    }
}

void RaylibRenderer::ClearScissor() {
    if (!scissorStack_.empty()) {
        EndScissorMode();
        scissorStack_.clear();
    }
}

render::Vec2 RaylibRenderer::LogicalSize() const {
    return {static_cast<float>(config_.logicalWidth), static_cast<float>(config_.logicalHeight)};
}

render::Rect RaylibRenderer::FinalPresentationRect() const {
    const float outputWidth = static_cast<float>(std::max<std::uint32_t>(1, config_.outputWidth));
    const float outputHeight = static_cast<float>(std::max<std::uint32_t>(1, config_.outputHeight));
    const float logicalWidth = static_cast<float>(std::max<std::uint32_t>(1, config_.logicalWidth));
    const float logicalHeight = static_cast<float>(std::max<std::uint32_t>(1, config_.logicalHeight));
    const float scale = std::min(outputWidth / logicalWidth, outputHeight / logicalHeight);
    const float width = std::round(logicalWidth * scale);
    const float height = std::round(logicalHeight * scale);
    return {std::round((outputWidth - width) * 0.5f), std::round((outputHeight - height) * 0.5f), width, height};
}

void RaylibRenderer::SetOutputFilter(render::TextureFilter filter) {
    config_.outputFilter = filter;
    if (logicalTarget_.texture.id != 0) {
        ::SetTextureFilter(logicalTarget_.texture, toRaylibFilter(filter));
    }
}

render::RendererStats RaylibRenderer::Stats() const {
    return {
        config_.logicalWidth,
        config_.logicalHeight,
        config_.renderWidth,
        config_.renderHeight,
        config_.outputWidth,
        config_.outputHeight,
        false,
        false,
        1
    };
}

Texture2D* RaylibRenderer::raylibTexture(render::TextureHandle handle) {
    return findTexture(handle);
}

RenderTexture2D* RaylibRenderer::raylibRenderTarget(render::RenderTargetHandle handle) {
    auto it = targets_.find(handle.id);
    if (it == targets_.end()) {
        return nullptr;
    }
    return &it->second;
}

Texture2D* RaylibRenderer::findTexture(render::TextureHandle handle) {
    auto textureIt = textures_.find(handle.id);
    if (textureIt != textures_.end()) {
        return &textureIt->second;
    }
    auto targetIt = targetTextureOwners_.find(handle.id);
    if (targetIt != targetTextureOwners_.end()) {
        RenderTexture2D* target = raylibRenderTarget(render::RenderTargetHandle{targetIt->second});
        return target ? &target->texture : nullptr;
    }
    if (handle.id == logicalTargetTextureId_ && logicalTarget_.texture.id != 0) {
        return &logicalTarget_.texture;
    }
    return nullptr;
}

Font* RaylibRenderer::findFont(render::FontHandle handle) {
    auto it = fonts_.find(handle.id);
    if (it == fonts_.end()) {
        return nullptr;
    }
    return &it->second.font;
}

void RaylibRenderer::unloadLogicalTarget() {
    if (logicalTarget_.id != 0) {
        UnloadRenderTexture(logicalTarget_);
        logicalTarget_ = {};
    }
    logicalTargetTextureId_ = 0;
}

void RaylibRenderer::ensureLogicalTarget() {
    if (logicalTarget_.id != 0 &&
        logicalTarget_.texture.width == static_cast<int>(config_.renderWidth) &&
        logicalTarget_.texture.height == static_cast<int>(config_.renderHeight)) {
        return;
    }
    unloadLogicalTarget();
    logicalTarget_ = LoadRenderTexture(static_cast<int>(std::max(1u, config_.renderWidth)),
                                       static_cast<int>(std::max(1u, config_.renderHeight)));
    if (logicalTarget_.texture.id != 0) {
        ::SetTextureFilter(logicalTarget_.texture, toRaylibFilter(config_.outputFilter));
        logicalTargetTextureId_ = nextTextureId_++;
    }
}

}  // namespace wb
