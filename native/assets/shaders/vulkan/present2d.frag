#version 450

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
    float gamma;
    float blackLevel;
    float whiteLevel;
    float screenBorder;
    float scanlines;
    float crtCurve;
    float hdrEncoding;
    float hdrPaperWhiteNits;
    float hdrMaxNits;
    float hdrHighlightStrength;
} pc;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 sampleFrame(vec2 uv) {
    return texture(sourceTexture, clamp(uv, vec2(0.0), vec2(1.0))).rgb;
}

vec3 applyFxaa(vec2 uv) {
    vec2 texel = 1.0 / max(pc.textureSize, vec2(1.0));
    vec3 rgbNW = sampleFrame(uv + vec2(-1.0, -1.0) * texel);
    vec3 rgbNE = sampleFrame(uv + vec2( 1.0, -1.0) * texel);
    vec3 rgbSW = sampleFrame(uv + vec2(-1.0,  1.0) * texel);
    vec3 rgbSE = sampleFrame(uv + vec2( 1.0,  1.0) * texel);
    vec3 rgbM = sampleFrame(uv);
    float lumaNW = luma(rgbNW);
    float lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW);
    float lumaSE = luma(rgbSE);
    float lumaM = luma(rgbM);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    if (lumaMax - lumaMin < 0.03125) {
        return rgbM;
    }
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.0078125, 0.0009765625);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin * texel, -8.0 * texel, 8.0 * texel);
    vec3 rgbA = 0.5 * (
        sampleFrame(uv + dir * (1.0 / 3.0 - 0.5)) +
        sampleFrame(uv + dir * (2.0 / 3.0 - 0.5))
    );
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        sampleFrame(uv + dir * -0.5) +
        sampleFrame(uv + dir * 0.5)
    );
    float lumaB = luma(rgbB);
    return (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
}

float randomNoise(vec2 value) {
    return fract(sin(dot(value, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 applyCurve(vec2 uv) {
    if (pc.crtCurve <= 0.0) {
        return uv;
    }
    vec2 centered = uv * 2.0 - 1.0;
    centered += centered * abs(centered.yx) * pc.crtCurve;
    return centered * 0.5 + 0.5;
}

vec3 applySharpness(vec2 uv, vec3 color) {
    float amount = pc.sharpness - 1.0;
    if (abs(amount) <= 0.001) {
        return color;
    }
    vec2 texel = 1.0 / max(pc.textureSize, vec2(1.0));
    vec3 blur = (
        sampleFrame(uv + vec2(-1.0,  0.0) * texel) +
        sampleFrame(uv + vec2( 1.0,  0.0) * texel) +
        sampleFrame(uv + vec2( 0.0, -1.0) * texel) +
        sampleFrame(uv + vec2( 0.0,  1.0) * texel)
    ) * 0.25;
    if (amount > 0.0) {
        return clamp(color + (color - blur) * amount * 0.72, 0.0, 1.0);
    }
    return mix(blur, color, pc.sharpness);
}

vec3 applyCrt(vec3 color, vec2 uv) {
    if (pc.crtStrength <= 0.0) {
        return color;
    }
    float scan = 0.92 + 0.08 * sin(uv.y * pc.textureSize.y * 3.14159265);
    vec2 centered = uv * 2.0 - 1.0;
    float vignette = 1.0 - dot(centered, centered) * 0.16 * pc.crtStrength;
    vec3 shifted = vec3(color.r * 1.012, color.g, color.b * 0.988);
    color = mix(color, shifted, pc.crtStrength);
    color *= mix(1.0, scan, pc.crtStrength);
    color *= clamp(vignette, 0.72, 1.0);
    return color;
}

vec3 srgbToLinear(vec3 color) {
    bvec3 cutoff = lessThanEqual(color, vec3(0.04045));
    vec3 low = color / 12.92;
    vec3 high = pow((color + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
}

vec3 linearSrgbToBt2020(vec3 color) {
    return mat3(
        0.6274040, 0.0690970, 0.0163916,
        0.3292820, 0.9195400, 0.0880132,
        0.0433136, 0.0113612, 0.8955950
    ) * color;
}

vec3 expandHdrHighlights(vec3 linearColor) {
    float paperWhite = max(pc.hdrPaperWhiteNits, 80.0);
    float peakRatio = max(1.0, pc.hdrMaxNits / paperWhite);
    float channelMax = max(linearColor.r, max(linearColor.g, linearColor.b));
    float channelMin = min(linearColor.r, min(linearColor.g, linearColor.b));
    float saturation = (channelMax - channelMin) / max(channelMax, 0.0001);
    float highlightMask = smoothstep(0.45, 0.95, channelMax) * smoothstep(0.08, 0.65, saturation);
    float expansion = clamp(pc.hdrHighlightStrength, 0.0, 1.0) * highlightMask;
    return min(linearColor * mix(1.0, peakRatio, expansion), vec3(peakRatio));
}

vec3 pqEncode(vec3 linearBt2020) {
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    vec3 absoluteNits = clamp(
        linearBt2020 * max(pc.hdrPaperWhiteNits, 80.0),
        vec3(0.0),
        vec3(max(pc.hdrMaxNits, pc.hdrPaperWhiteNits))
    );
    vec3 normalized = clamp(absoluteNits / 10000.0, 0.0, 1.0);
    vec3 powered = pow(normalized, vec3(m1));
    return pow((c1 + c2 * powered) / (1.0 + c3 * powered), vec3(m2));
}

vec3 encodeOutput(vec3 srgbColor) {
    if (pc.hdrEncoding < 0.5) {
        return clamp(srgbColor, 0.0, 1.0);
    }
    if (pc.hdrEncoding > 2.5) {
        return srgbToLinear(clamp(srgbColor, 0.0, 1.0));
    }
    vec3 linear = expandHdrHighlights(srgbToLinear(max(srgbColor, vec3(0.0))));
    if (pc.hdrEncoding < 1.5) {
        return linear * (pc.hdrPaperWhiteNits / 80.0);
    }
    return pqEncode(max(linearSrgbToBt2020(linear), vec3(0.0)));
}

void main() {
    vec2 uv = applyCurve(fragTexCoord);
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 source = texture(sourceTexture, uv);
    vec3 color = pc.useFxaa > 0.5 ? applyFxaa(uv) : source.rgb;
    color = applySharpness(uv, color);
    color = (color - 0.5) * pc.contrast + 0.5;
    color *= pc.brightness;
    color = pow(max(color, vec3(0.0)), vec3(1.0 / max(pc.gamma, 0.001)));
    color = color * pc.whiteLevel + vec3(pc.blackLevel);
    color = applyCrt(color, uv);

    if (pc.noiseAmount > 0.0) {
        float noise = randomNoise(floor(uv * pc.textureSize) + floor(pc.time * 45.0)) - 0.5;
        color += noise * pc.noiseAmount;
    }
    if (pc.scanlines > 0.0) {
        float line = 0.5 + 0.5 * sin(uv.y * pc.textureSize.y * 3.14159265);
        color *= 1.0 - (1.0 - line) * pc.scanlines;
    }
    if (pc.screenBorder > 0.0) {
        vec2 centered = fragTexCoord * 2.0 - 1.0;
        float vignette = smoothstep(1.32, 0.42, dot(centered, centered));
        color *= mix(1.0, vignette, pc.screenBorder);
    }

    outColor = vec4(encodeOutput(color), source.a) * fragColor;
}
