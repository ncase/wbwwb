#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 uTextureSize;
uniform float uTime;
uniform int uUseFxaa;
uniform float uCrtStrength;
uniform float uNoiseAmount;
uniform float uBrightness;
uniform float uContrast;
uniform float uSharpness;
uniform float uGamma;
uniform float uBlackLevel;
uniform float uWhiteLevel;
uniform float uScreenBorder;
uniform float uScanlines;
uniform float uCrtCurve;

out vec4 finalColor;

float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 sampleFrame(vec2 uv) {
    return texture(texture0, clamp(uv, vec2(0.0), vec2(1.0))).rgb;
}

vec3 applyFxaa(vec2 uv) {
    vec2 texel = 1.0 / max(uTextureSize, vec2(1.0));

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
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

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
    if (uCrtCurve <= 0.0) {
        return uv;
    }
    vec2 centered = uv * 2.0 - 1.0;
    centered += centered * abs(centered.yx) * uCrtCurve;
    return centered * 0.5 + 0.5;
}

vec3 applySharpness(vec2 uv, vec3 color) {
    float amount = uSharpness - 1.0;
    if (abs(amount) <= 0.001) {
        return color;
    }

    vec2 texel = 1.0 / max(uTextureSize, vec2(1.0));
    vec3 blur = (
        sampleFrame(uv + vec2(-1.0,  0.0) * texel) +
        sampleFrame(uv + vec2( 1.0,  0.0) * texel) +
        sampleFrame(uv + vec2( 0.0, -1.0) * texel) +
        sampleFrame(uv + vec2( 0.0,  1.0) * texel)
    ) * 0.25;

    if (amount > 0.0) {
        return clamp(color + (color - blur) * amount * 0.72, 0.0, 1.0);
    }
    return mix(blur, color, uSharpness);
}

vec3 applyCrt(vec3 color, vec2 uv) {
    if (uCrtStrength <= 0.0) {
        return color;
    }

    float scan = 0.92 + 0.08 * sin(uv.y * uTextureSize.y * 3.14159265);
    vec2 centered = uv * 2.0 - 1.0;
    float vignette = 1.0 - dot(centered, centered) * 0.16 * uCrtStrength;
    vec3 shifted = vec3(color.r * 1.012, color.g, color.b * 0.988);
    color = mix(color, shifted, uCrtStrength);
    color *= mix(1.0, scan, uCrtStrength);
    color *= clamp(vignette, 0.72, 1.0);
    return color;
}

void main() {
    vec2 uv = applyCurve(fragTexCoord);
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0) * colDiffuse * fragColor;
        return;
    }

    vec4 source = texture(texture0, uv);
    vec3 color = (uUseFxaa != 0) ? applyFxaa(uv) : source.rgb;

    color = applySharpness(uv, color);
    color = (color - 0.5) * uContrast + 0.5;
    color *= uBrightness;
    color = pow(max(color, vec3(0.0)), vec3(1.0 / max(uGamma, 0.001)));
    color = color * uWhiteLevel + vec3(uBlackLevel);
    color = applyCrt(color, uv);

    if (uNoiseAmount > 0.0) {
        float noise = randomNoise(floor(uv * uTextureSize) + floor(uTime * 45.0)) - 0.5;
        color += noise * uNoiseAmount;
    }

    if (uScanlines > 0.0) {
        float line = 0.5 + 0.5 * sin(uv.y * uTextureSize.y * 3.14159265);
        color *= 1.0 - (1.0 - line) * uScanlines;
    }

    if (uScreenBorder > 0.0) {
        vec2 centered = fragTexCoord * 2.0 - 1.0;
        float vignette = smoothstep(1.32, 0.42, dot(centered, centered));
        color *= mix(1.0, vignette, uScreenBorder);
    }

    finalColor = vec4(clamp(color, 0.0, 1.0), source.a) * colDiffuse * fragColor;
}
