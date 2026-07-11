#pragma once

namespace wb {

enum class RenderBackendKind {
    RaylibOpenGL,
    Vulkan
};

RenderBackendKind configuredRenderBackend();
const char* configuredRenderBackendName();
bool configuredRenderBackendHasNativeHdrOutput();

}  // namespace wb
