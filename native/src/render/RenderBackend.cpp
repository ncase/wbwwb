#include "render/RenderBackend.h"

namespace wb {

RenderBackendKind configuredRenderBackend() {
#if defined(WBWWB_RENDER_BACKEND_VULKAN)
    return RenderBackendKind::Vulkan;
#else
    return RenderBackendKind::RaylibOpenGL;
#endif
}

const char* configuredRenderBackendName() {
#if defined(WBWWB_RENDER_BACKEND_VULKAN)
    return "vulkan";
#else
    return "raylib-opengl";
#endif
}

bool configuredRenderBackendHasNativeHdrOutput() {
    return false;
}

}  // namespace wb
