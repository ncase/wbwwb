#version 450

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in vec2 fragUvMin;
layout(location = 3) flat in vec2 fragUvMax;
layout(location = 0) out vec4 outColor;

float catmullRomWeight(float distanceFromSample) {
    float x = abs(distanceFromSample);
    if (x < 1.0) {
        return 1.0 - 2.5 * x * x + 1.5 * x * x * x;
    }
    if (x < 2.0) {
        return 2.0 - 4.0 * x + 2.5 * x * x - 0.5 * x * x * x;
    }
    return 0.0;
}

vec4 samplePremultipliedTexel(vec2 texel, vec2 textureDimensions) {
    vec2 uv = (texel + vec2(0.5)) / textureDimensions;
    uv = clamp(uv, fragUvMin, fragUvMax);
    vec4 sampleColor = texture(sourceTexture, uv);
    sampleColor.rgb *= sampleColor.a;
    return sampleColor;
}

vec4 sampleHighQuality(vec2 uv) {
    vec2 textureDimensions = vec2(textureSize(sourceTexture, 0));
    vec2 texelPosition = uv * textureDimensions - vec2(0.5);
    vec2 baseTexel = floor(texelPosition);
    vec2 fraction = texelPosition - baseTexel;
    vec4 filtered = vec4(0.0);
    float totalWeight = 0.0;

    for (int y = -1; y <= 2; ++y) {
        float weightY = catmullRomWeight(float(y) - fraction.y);
        for (int x = -1; x <= 2; ++x) {
            float weight = catmullRomWeight(float(x) - fraction.x) * weightY;
            filtered += samplePremultipliedTexel(baseTexel + vec2(x, y), textureDimensions) * weight;
            totalWeight += weight;
        }
    }

    filtered /= max(totalWeight, 0.00001);
    float alpha = clamp(filtered.a, 0.0, 1.0);
    vec3 color = alpha > 0.00001 ? clamp(filtered.rgb / alpha, 0.0, 1.0) : vec3(0.0);
    return vec4(color, alpha);
}

void main() {
    outColor = sampleHighQuality(fragTexCoord) * fragColor;
}
