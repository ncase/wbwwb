#pragma once

#include "VulkanRenderer2D.h"
#include "VulkanTexture.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>

#ifdef DrawText
#undef DrawText
#endif

namespace wbwwb::vulkan {

class VulkanFont final {
public:
    struct TextExtent {
        float width = 0.0f;
        float height = 0.0f;
    };

    void Create(const VulkanDevice& device,
                VkCommandPool commandPool,
                VkQueue graphicsQueue,
                const std::filesystem::path& fontPath,
                int pixelHeight);
    void Destroy(VkDevice device);

    void DrawText(VulkanRenderer2D& renderer,
                  VkDescriptorSet descriptorSet,
                  std::string_view text,
                  float x,
                  float y,
                  float size,
                  VulkanRenderer2D::Color color) const;
    [[nodiscard]] TextExtent MeasureText(std::string_view text, float size) const;

    [[nodiscard]] VkImageView ImageView() const noexcept { return atlas_.ImageView(); }
    [[nodiscard]] VkSampler Sampler() const noexcept { return atlas_.Sampler(); }
    [[nodiscard]] std::uint32_t AtlasWidth() const noexcept { return atlas_.Width(); }
    [[nodiscard]] std::uint32_t AtlasHeight() const noexcept { return atlas_.Height(); }
    [[nodiscard]] int PixelHeight() const noexcept { return pixelHeight_; }
    [[nodiscard]] bool Ready() const noexcept { return atlas_.ImageView() != VK_NULL_HANDLE; }

private:
    struct Glyph {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float advance = 0.0f;
    };

    [[nodiscard]] const Glyph& GlyphFor(std::uint32_t codepoint) const noexcept;

    VulkanTexture atlas_;
    std::array<Glyph, 384> glyphs_{};
    Glyph fallback_{};
    int pixelHeight_ = 0;
    float lineHeight_ = 0.0f;
};

} // namespace wbwwb::vulkan
