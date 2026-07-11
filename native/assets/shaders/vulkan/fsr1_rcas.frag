#version 450
#extension GL_GOOGLE_include_directive : require

// AMD FidelityFX Super Resolution 1.0.2, MIT licensed.
// The implementation is provided by the pinned GPUOpen FidelityFX-FSR dependency.

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

layout(push_constant) uniform PushConstants {
    vec2 viewportSize;
    vec2 textureSize;
    float time;
    float useFxaa;
    float crtStrength;
    float noiseAmount;
    float brightness;
    float contrast;
    float sharpness;
} pc;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

#define A_GPU 1
#define A_GLSL 1
#include "ffx_a.h"

#define FSR_RCAS_F 1
AF4 FsrRcasLoadF(ASU2 p) {
    ASU2 maxCoord = ASU2(textureSize(sourceTexture, 0) - ivec2(1));
    return texelFetch(sourceTexture, clamp(p, ASU2(0), maxCoord), 0);
}
void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}
#include "ffx_fsr1.h"

void main() {
    AU4 con;
    FsrRcasCon(con, clamp(pc.sharpness, 0.0, 2.0));

    AF3 color;
    AU2 outputPixel = AU2(clamp(
        floor(fragTexCoord * pc.viewportSize),
        vec2(0.0),
        pc.viewportSize - vec2(1.0)
    ));
    FsrRcasF(color.r, color.g, color.b, outputPixel, con);
    outColor = vec4(color, 1.0) * fragColor;
}
