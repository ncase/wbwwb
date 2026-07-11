#include "render/raylib/RaylibFramePipeline.h"

#include "AssetManager.h"
#include "Constants.h"
#include "Localization.h"
#include "render/raylib/RaylibRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>

#include "rlgl.h"

namespace wb {
namespace {

constexpr float kMinViewportScale = 0.001f;
constexpr std::array<int, 4> kMsaaFallbackSamples{{16, 8, 4, 2}};

#if defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY __stdcall
#elif !defined(APIENTRY)
#define APIENTRY
#endif

using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLbitfield = unsigned int;

constexpr GLenum kGlFramebuffer = 0x8D40;
constexpr GLenum kGlRenderbuffer = 0x8D41;
constexpr GLenum kGlReadFramebuffer = 0x8CA8;
constexpr GLenum kGlDrawFramebuffer = 0x8CA9;
constexpr GLenum kGlColorAttachment0 = 0x8CE0;
constexpr GLenum kGlFramebufferComplete = 0x8CD5;
constexpr GLenum kGlRgba8 = 0x8058;
constexpr GLenum kGlMaxSamples = 0x8D57;
constexpr GLenum kGlMultisample = 0x809D;
constexpr GLenum kGlNearest = 0x2600;
constexpr GLbitfield kGlColorBufferBit = 0x00004000;

extern "C" void* rlGetProcAddress(const char* procName);

struct GlMsaaApi {
    using GetIntegerv = void (APIENTRY *)(GLenum, GLint*);
    using Enable = void (APIENTRY *)(GLenum);
    using GenFramebuffers = void (APIENTRY *)(GLsizei, GLuint*);
    using BindFramebuffer = void (APIENTRY *)(GLenum, GLuint);
    using DeleteFramebuffers = void (APIENTRY *)(GLsizei, const GLuint*);
    using GenRenderbuffers = void (APIENTRY *)(GLsizei, GLuint*);
    using BindRenderbuffer = void (APIENTRY *)(GLenum, GLuint);
    using RenderbufferStorageMultisample = void (APIENTRY *)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    using FramebufferRenderbuffer = void (APIENTRY *)(GLenum, GLenum, GLenum, GLuint);
    using CheckFramebufferStatus = GLenum (APIENTRY *)(GLenum);
    using DeleteRenderbuffers = void (APIENTRY *)(GLsizei, const GLuint*);
    using BlitFramebuffer = void (APIENTRY *)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);

    bool tried = false;
    bool ready = false;
    GetIntegerv getIntegerv = nullptr;
    Enable enable = nullptr;
    GenFramebuffers genFramebuffers = nullptr;
    BindFramebuffer bindFramebuffer = nullptr;
    DeleteFramebuffers deleteFramebuffers = nullptr;
    GenRenderbuffers genRenderbuffers = nullptr;
    BindRenderbuffer bindRenderbuffer = nullptr;
    RenderbufferStorageMultisample renderbufferStorageMultisample = nullptr;
    FramebufferRenderbuffer framebufferRenderbuffer = nullptr;
    CheckFramebufferStatus checkFramebufferStatus = nullptr;
    DeleteRenderbuffers deleteRenderbuffers = nullptr;
    BlitFramebuffer blitFramebuffer = nullptr;
};

GlMsaaApi& glMsaaApi() {
    static GlMsaaApi api;
    return api;
}

template <typename T>
T loadGlProc(const char* name) {
    return reinterpret_cast<T>(rlGetProcAddress(name));
}

bool ensureGlMsaaApi() {
    GlMsaaApi& api = glMsaaApi();
    if (api.tried) {
        return api.ready;
    }

    api.tried = true;
    api.getIntegerv = loadGlProc<GlMsaaApi::GetIntegerv>("glGetIntegerv");
    api.enable = loadGlProc<GlMsaaApi::Enable>("glEnable");
    api.genFramebuffers = loadGlProc<GlMsaaApi::GenFramebuffers>("glGenFramebuffers");
    api.bindFramebuffer = loadGlProc<GlMsaaApi::BindFramebuffer>("glBindFramebuffer");
    api.deleteFramebuffers = loadGlProc<GlMsaaApi::DeleteFramebuffers>("glDeleteFramebuffers");
    api.genRenderbuffers = loadGlProc<GlMsaaApi::GenRenderbuffers>("glGenRenderbuffers");
    api.bindRenderbuffer = loadGlProc<GlMsaaApi::BindRenderbuffer>("glBindRenderbuffer");
    api.renderbufferStorageMultisample = loadGlProc<GlMsaaApi::RenderbufferStorageMultisample>("glRenderbufferStorageMultisample");
    api.framebufferRenderbuffer = loadGlProc<GlMsaaApi::FramebufferRenderbuffer>("glFramebufferRenderbuffer");
    api.checkFramebufferStatus = loadGlProc<GlMsaaApi::CheckFramebufferStatus>("glCheckFramebufferStatus");
    api.deleteRenderbuffers = loadGlProc<GlMsaaApi::DeleteRenderbuffers>("glDeleteRenderbuffers");
    api.blitFramebuffer = loadGlProc<GlMsaaApi::BlitFramebuffer>("glBlitFramebuffer");

    api.ready = api.getIntegerv &&
                api.genFramebuffers &&
                api.bindFramebuffer &&
                api.deleteFramebuffers &&
                api.genRenderbuffers &&
                api.bindRenderbuffer &&
                api.renderbufferStorageMultisample &&
                api.framebufferRenderbuffer &&
                api.checkFramebufferStatus &&
                api.deleteRenderbuffers &&
                api.blitFramebuffer;
    if (!api.ready) {
        TraceLog(LOG_WARNING, "MSAA: OpenGL multisample framebuffer functions are unavailable");
    } else if (api.enable) {
        api.enable(kGlMultisample);
    }
    return api.ready;
}

int normalizeMsaaRequest(int samples) {
    if (samples <= 0) {
        return 0;
    }
    if (samples >= 16) {
        return 16;
    }
    if (samples >= 8) {
        return 8;
    }
    if (samples >= 4) {
        return 4;
    }
    return 2;
}

Rectangle toRaylib(render::Rect value) {
    return {value.x, value.y, value.width, value.height};
}

}  // namespace

RaylibFramePipeline::RaylibFramePipeline(RaylibRenderer& renderer)
    : renderer_(renderer) {
}

RaylibFramePipeline::~RaylibFramePipeline() {
    UnloadPostShader();
    UnloadMsaaTarget();
    if (target_.id != 0) {
        UnloadRenderTexture(target_);
    }
}

void RaylibFramePipeline::UpdateViewport(const GameSettings& settings) {
    const float screenW = static_cast<float>(std::max(1, GetScreenWidth()));
    const float screenH = static_cast<float>(std::max(1, GetScreenHeight()));
    const float gameW = static_cast<float>(kGameWidth);
    const float gameH = static_cast<float>(kGameHeight);
    const float safeArea = std::clamp(settings.safeArea, 0.9f, 1.0f);
    const float areaW = std::max(1.0f, screenW * safeArea);
    const float areaH = std::max(1.0f, screenH * safeArea);
    const float areaX = (screenW - areaW) / 2.0f;
    const float areaY = (screenH - areaH) / 2.0f;
    const float scaleX = areaW / gameW;
    const float scaleY = areaH / gameH;
    float scale = settings.aspectFit == 1 ? std::max(scaleX, scaleY) : std::min(scaleX, scaleY);
    if (settings.integerScaling && scale >= 1.0f) {
        scale = settings.aspectFit == 1 ? std::ceil(scale) : std::floor(scale);
    }
    scale = std::max(scale, kMinViewportScale);
    viewport_.width = std::max(1.0f, std::round(gameW * scale));
    viewport_.height = std::max(1.0f, std::round(gameH * scale));
    viewport_.x = std::round(areaX + (areaW - viewport_.width) / 2.0f);
    viewport_.y = std::round(areaY + (areaH - viewport_.height) / 2.0f);

    UpdateRenderTarget(settings);
}

void RaylibFramePipeline::UpdateRenderTarget(const GameSettings& settings) {
    const WindowSizeOption& selected = settings.currentWindowSize();
    const int desiredWidth = std::clamp(selected.width, 1, 16384);
    const int desiredHeight = std::clamp(selected.height, 1, 16384);
    const int filter = settings.outputSmoothing == 1 ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT;
    if (target_.id != 0 && renderWidth_ == desiredWidth && renderHeight_ == desiredHeight) {
        SetTextureFilter(target_.texture, filter);
        logicalScale_ = std::max(kMinViewportScale, std::min(
            static_cast<float>(renderWidth_) / static_cast<float>(kGameWidth),
            static_cast<float>(renderHeight_) / static_cast<float>(kGameHeight)
        ));
        logicalViewport_ = {0.0f, 0.0f, static_cast<float>(renderWidth_), static_cast<float>(renderHeight_)};
        UpdateMsaaTarget(settings);
        return;
    }

    UnloadMsaaTarget();
    if (target_.id != 0) {
        UnloadRenderTexture(target_);
    }

    renderWidth_ = desiredWidth;
    renderHeight_ = desiredHeight;
    target_ = LoadRenderTexture(renderWidth_, renderHeight_);
    if (target_.id != 0) {
        SetTextureFilter(target_.texture, filter);
        TraceLog(LOG_INFO, "Selected-resolution render target: %ix%i", renderWidth_, renderHeight_);
    }
    logicalScale_ = std::max(kMinViewportScale, std::min(
        static_cast<float>(renderWidth_) / static_cast<float>(kGameWidth),
        static_cast<float>(renderHeight_) / static_cast<float>(kGameHeight)
    ));
    logicalViewport_ = {0.0f, 0.0f, static_cast<float>(renderWidth_), static_cast<float>(renderHeight_)};
    UpdateMsaaTarget(settings);
}

void RaylibFramePipeline::UpdateMsaaTarget(const GameSettings& settings) {
    const int requestedSamples = normalizeMsaaRequest(settings.msaaSamples);
    if (target_.id == 0 || requestedSamples <= 0) {
        UnloadMsaaTarget();
        msaaUnavailableWarned_ = false;
        return;
    }

    if (msaaTarget_.framebuffer != 0 &&
        msaaTarget_.width == renderWidth_ &&
        msaaTarget_.height == renderHeight_ &&
        msaaTarget_.requestedSamples == requestedSamples) {
        return;
    }

    UnloadMsaaTarget();
    if (!LoadMsaaTarget(renderWidth_, renderHeight_, requestedSamples) && !msaaUnavailableWarned_) {
        TraceLog(LOG_WARNING, "MSAA: falling back to non-multisampled rendering");
        msaaUnavailableWarned_ = true;
    }
}

bool RaylibFramePipeline::LoadMsaaTarget(int width, int height, int requestedSamples) {
    if (!ensureGlMsaaApi()) {
        return false;
    }

    GlMsaaApi& gl = glMsaaApi();
    GLint maxSamples = 0;
    gl.getIntegerv(kGlMaxSamples, &maxSamples);
    if (maxSamples < 2) {
        TraceLog(LOG_WARNING, "MSAA: driver reports no supported multisample renderbuffer samples");
        return false;
    }

    for (int samples : kMsaaFallbackSamples) {
        if (samples > requestedSamples || samples > maxSamples) {
            continue;
        }

        GLuint framebuffer = 0;
        GLuint colorBuffer = 0;
        gl.genFramebuffers(1, &framebuffer);
        gl.bindFramebuffer(kGlFramebuffer, framebuffer);
        gl.genRenderbuffers(1, &colorBuffer);
        gl.bindRenderbuffer(kGlRenderbuffer, colorBuffer);
        gl.renderbufferStorageMultisample(kGlRenderbuffer, samples, kGlRgba8, width, height);
        gl.framebufferRenderbuffer(kGlFramebuffer, kGlColorAttachment0, kGlRenderbuffer, colorBuffer);
        const GLenum status = gl.checkFramebufferStatus(kGlFramebuffer);
        gl.bindRenderbuffer(kGlRenderbuffer, 0);
        gl.bindFramebuffer(kGlFramebuffer, 0);

        if (framebuffer != 0 && colorBuffer != 0 && status == kGlFramebufferComplete) {
            msaaTarget_.framebuffer = framebuffer;
            msaaTarget_.colorBuffer = colorBuffer;
            msaaTarget_.width = width;
            msaaTarget_.height = height;
            msaaTarget_.samples = samples;
            msaaTarget_.requestedSamples = requestedSamples;
            msaaUnavailableWarned_ = false;
            if (samples == requestedSamples) {
                TraceLog(LOG_INFO, "MSAA: using offscreen %ix target", samples);
            } else {
                TraceLog(LOG_INFO, "MSAA: requested %ix, using %ix supported target", requestedSamples, samples);
            }
            return true;
        }

        if (colorBuffer != 0) {
            gl.deleteRenderbuffers(1, &colorBuffer);
        }
        if (framebuffer != 0) {
            gl.deleteFramebuffers(1, &framebuffer);
        }
    }

    TraceLog(LOG_WARNING, "MSAA: could not create an offscreen multisample target for requested %ix", requestedSamples);
    return false;
}

void RaylibFramePipeline::UnloadMsaaTarget() {
    if (msaaTarget_.framebuffer == 0 && msaaTarget_.colorBuffer == 0) {
        msaaTarget_ = {};
        return;
    }

    GlMsaaApi& gl = glMsaaApi();
    if (gl.ready) {
        if (msaaTarget_.colorBuffer != 0) {
            const GLuint colorBuffer = msaaTarget_.colorBuffer;
            gl.deleteRenderbuffers(1, &colorBuffer);
        }
        if (msaaTarget_.framebuffer != 0) {
            const GLuint framebuffer = msaaTarget_.framebuffer;
            gl.deleteFramebuffers(1, &framebuffer);
        }
    }
    msaaTarget_ = {};
}

bool RaylibFramePipeline::UsingMsaaTarget() const {
    return msaaTarget_.framebuffer != 0 && msaaTarget_.colorBuffer != 0 && msaaTarget_.samples > 1;
}

void RaylibFramePipeline::ResolveMsaaTarget() {
    if (!UsingMsaaTarget() || target_.id == 0) {
        return;
    }

    GlMsaaApi& gl = glMsaaApi();
    if (!gl.ready) {
        return;
    }

    gl.bindFramebuffer(kGlReadFramebuffer, msaaTarget_.framebuffer);
    gl.bindFramebuffer(kGlDrawFramebuffer, target_.id);
    gl.blitFramebuffer(0, 0, renderWidth_, renderHeight_, 0, 0, renderWidth_, renderHeight_, kGlColorBufferBit, kGlNearest);
    gl.bindFramebuffer(kGlReadFramebuffer, 0);
    gl.bindFramebuffer(kGlDrawFramebuffer, 0);
}

void RaylibFramePipeline::BeginLogicalDraw(const GameSettings& settings) {
    UpdateViewport(settings);

    RenderTexture2D drawTarget = target_;
    if (UsingMsaaTarget()) {
        drawTarget = {};
        drawTarget.id = msaaTarget_.framebuffer;
        drawTarget.texture.width = renderWidth_;
        drawTarget.texture.height = renderHeight_;
    }
    BeginTextureMode(drawTarget);
    ClearBackground(BLACK);
    renderer_.BeginExternalLogicalRender();
    rlPushMatrix();
    rlScalef(
        static_cast<float>(renderWidth_) / static_cast<float>(kGameWidth),
        static_cast<float>(renderHeight_) / static_cast<float>(kGameHeight),
        1.0f
    );
}

void RaylibFramePipeline::EndLogicalDraw(const GameSettings& settings,
                                         AssetManager* assets,
                                         const Localization* localization,
                                         float postTime) {
    if (settings.fpsCounter && assets != nullptr && localization != nullptr) {
        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "%s: %d", localization->tr("hud.fps").c_str(), GetFPS());
        renderer_.DrawText({
            assets->fontHandle(64),
            buffer,
            {8.0f, 7.0f},
            16.0f,
            {235, 235, 235, 230},
            1.0f
        });
    }
    renderer_.EndExternalLogicalRender();
    rlPopMatrix();
    EndTextureMode();
    ResolveMsaaTarget();

    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle source{0.0f, 0.0f, static_cast<float>(renderWidth_), -static_cast<float>(renderHeight_)};
    if (assets != nullptr && PostProcessingRequested(settings) && EnsurePostShader(*assets)) {
        UpdatePostShaderUniforms(settings, postTime);
        BeginShaderMode(postShader_);
        DrawTexturePro(target_.texture, source, toRaylib(viewport_), {0.0f, 0.0f}, 0.0f, WHITE);
        EndShaderMode();
    } else {
        DrawTexturePro(target_.texture, source, toRaylib(viewport_), {0.0f, 0.0f}, 0.0f, WHITE);
    }
    EndDrawing();
}

bool RaylibFramePipeline::PostProcessingRequested(const GameSettings& settings) const {
    return settings.fxaa ||
           settings.crtFilter > 0 ||
           settings.screenNoise > 0 ||
           settings.screenBorder > 0 ||
           settings.scanlines > 0 ||
           settings.crtCurve > 0 ||
           std::abs(settings.sharpness - 1.0f) > 0.001f ||
           std::abs(settings.gamma - 1.0f) > 0.001f ||
           std::abs(settings.brightness - 1.0f) > 0.001f ||
           std::abs(settings.contrast - 1.0f) > 0.001f ||
           std::abs(settings.blackLevel) > 0.001f ||
           std::abs(settings.whiteLevel - 1.0f) > 0.001f;
}

bool RaylibFramePipeline::EnsurePostShader(AssetManager& assets) {
    if (postShaderTried_) {
        return postShaderReady_;
    }

    postShaderTried_ = true;
    const std::string shaderPath = assets.path("native/assets/shaders/postprocess.fs");
    postShader_ = LoadShader(nullptr, shaderPath.c_str());
    if (postShader_.id == 0) {
        std::cerr << "Warning: visual post-processing shader could not load from "
                  << shaderPath << ". Final-frame visual effects are disabled for this run.\n";
        postShaderReady_ = false;
        return false;
    }

    postTextureSizeLoc_ = GetShaderLocation(postShader_, "uTextureSize");
    postTimeLoc_ = GetShaderLocation(postShader_, "uTime");
    postUseFxaaLoc_ = GetShaderLocation(postShader_, "uUseFxaa");
    postCrtStrengthLoc_ = GetShaderLocation(postShader_, "uCrtStrength");
    postNoiseAmountLoc_ = GetShaderLocation(postShader_, "uNoiseAmount");
    postBrightnessLoc_ = GetShaderLocation(postShader_, "uBrightness");
    postContrastLoc_ = GetShaderLocation(postShader_, "uContrast");
    postSharpnessLoc_ = GetShaderLocation(postShader_, "uSharpness");
    postGammaLoc_ = GetShaderLocation(postShader_, "uGamma");
    postBlackLevelLoc_ = GetShaderLocation(postShader_, "uBlackLevel");
    postWhiteLevelLoc_ = GetShaderLocation(postShader_, "uWhiteLevel");
    postScreenBorderLoc_ = GetShaderLocation(postShader_, "uScreenBorder");
    postScanlinesLoc_ = GetShaderLocation(postShader_, "uScanlines");
    postCrtCurveLoc_ = GetShaderLocation(postShader_, "uCrtCurve");
    postShaderReady_ = true;
    return true;
}

void RaylibFramePipeline::UnloadPostShader() {
    if (postShader_.id != 0) {
        UnloadShader(postShader_);
        postShader_ = {};
    }
    postShaderReady_ = false;
}

void RaylibFramePipeline::UpdatePostShaderUniforms(const GameSettings& settings, float postTime) {
    const Vector2 textureSize{static_cast<float>(std::max(1, renderWidth_)), static_cast<float>(std::max(1, renderHeight_))};
    const int useFxaa = settings.fxaa ? 1 : 0;
    const float flashScale = settings.reduceFlashing ? 0.35f : 1.0f;
    const float crtStrength = (settings.crtFilter == 2 ? 0.78f : settings.crtFilter == 1 ? 0.38f : 0.0f) * flashScale;
    const float noiseAmount = (settings.screenNoise == 3 ? 0.10f : settings.screenNoise == 2 ? 0.055f : settings.screenNoise == 1 ? 0.025f : 0.0f) * flashScale;
    const float brightness = std::clamp(settings.brightness, 0.5f, 1.5f);
    const float contrast = std::clamp(settings.contrast, 0.5f, 1.5f);
    const float sharpness = std::clamp(settings.sharpness, 0.0f, 2.0f);
    const float gamma = std::clamp(settings.gamma, 0.8f, 1.2f);
    const float blackLevel = std::clamp(settings.blackLevel, 0.0f, 0.2f);
    const float whiteLevel = std::clamp(settings.whiteLevel, 0.8f, 1.2f);
    const float screenBorder = settings.screenBorder == 2 ? 0.65f : settings.screenBorder == 1 ? 0.32f : 0.0f;
    const float scanlines = (settings.scanlines == 3 ? 0.34f : settings.scanlines == 2 ? 0.22f : settings.scanlines == 1 ? 0.12f : 0.0f) * flashScale;
    const float crtCurve = settings.crtCurve == 2 ? 0.11f : settings.crtCurve == 1 ? 0.055f : 0.0f;

    SetShaderValue(postShader_, postTextureSizeLoc_, &textureSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(postShader_, postTimeLoc_, &postTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postUseFxaaLoc_, &useFxaa, SHADER_UNIFORM_INT);
    SetShaderValue(postShader_, postCrtStrengthLoc_, &crtStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postNoiseAmountLoc_, &noiseAmount, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postBrightnessLoc_, &brightness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postContrastLoc_, &contrast, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postSharpnessLoc_, &sharpness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postGammaLoc_, &gamma, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postBlackLevelLoc_, &blackLevel, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postWhiteLevelLoc_, &whiteLevel, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postScreenBorderLoc_, &screenBorder, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postScanlinesLoc_, &scanlines, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader_, postCrtCurveLoc_, &crtCurve, SHADER_UNIFORM_FLOAT);
}

}  // namespace wb
