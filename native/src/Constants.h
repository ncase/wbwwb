#pragma once

namespace wb {

constexpr int kGameWidth = 960;
constexpr int kGameHeight = 540;
constexpr float kAssetTextureResolution = 2.0f;
constexpr float kBeat = 1.0f;
constexpr float kFrameRate = 60.0f;
constexpr const char* kTitle = "Native WBWWB C++ Port";

inline float ticks(float seconds) {
    return seconds;
}

}  // namespace wb
