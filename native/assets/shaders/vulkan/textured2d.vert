#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inUvMin;
layout(location = 4) in vec2 inUvMax;

layout(push_constant) uniform PushConstants {
    vec2 viewportSize;
} pc;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) flat out vec2 fragUvMin;
layout(location = 3) flat out vec2 fragUvMax;

void main() {
    vec2 normalized = inPosition / pc.viewportSize;
    vec2 clip = vec2(normalized.x * 2.0 - 1.0, 1.0 - normalized.y * 2.0);
    gl_Position = vec4(clip, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragUvMin = inUvMin;
    fragUvMax = inUvMax;
}
