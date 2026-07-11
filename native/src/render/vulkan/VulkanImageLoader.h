#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace wbwwb::vulkan {

struct LoadedImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

[[nodiscard]] LoadedImage LoadRgbaImage(const std::filesystem::path& path,
                                        std::uint32_t requestedWidth = 0,
                                        std::uint32_t requestedHeight = 0,
                                        bool nearest = false);

} // namespace wbwwb::vulkan
