#version 450
#extension GL_GOOGLE_include_directive : require

// AMD FidelityFX Super Resolution 1.0.2, MIT licensed.
// The implementation is provided by the pinned GPUOpen FidelityFX-FSR dependency.

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

layout(push_constant) uniform PushConstants {
    vec2 viewportSize;
    vec2 textureSize;
} pc;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

#define A_GPU 1
#define A_GLSL 1
#include "ffx_a.h"

#define FSR_EASU_F 1
AF4 FsrEasuRF(AF2 p) { return textureGather(sourceTexture, p, 0); }
AF4 FsrEasuGF(AF2 p) { return textureGather(sourceTexture, p, 1); }
AF4 FsrEasuBF(AF2 p) { return textureGather(sourceTexture, p, 2); }
#include "ffx_fsr1.h"

void main() {
    AU4 con0;
    AU4 con1;
    AU4 con2;
    AU4 con3;
    FsrEasuCon(
        con0,
        con1,
        con2,
        con3,
        pc.textureSize.x,
        pc.textureSize.y,
        pc.textureSize.x,
        pc.textureSize.y,
        pc.viewportSize.x,
        pc.viewportSize.y
    );

    AF3 color;
    AU2 outputPixel = AU2(clamp(
        floor(fragTexCoord * pc.viewportSize),
        vec2(0.0),
        pc.viewportSize - vec2(1.0)
    ));
    FsrEasuF(color, outputPixel, con0, con1, con2, con3);
    outColor = vec4(color, 1.0) * fragColor;
}
