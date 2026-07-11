#pragma once

// Native WBWWB C++ Port Vulkan graphics pipeline resources.

#include <vulkan/vulkan.h>

#include <filesystem>

namespace wbwwb::vulkan {

class VulkanPipeline final {
public:
    void CreateSolid2D(VkDevice device,
                       VkRenderPass renderPass,
                       VkExtent2D viewportExtent,
                       const std::filesystem::path& shaderDirectory,
                       const std::filesystem::path& texturedFragmentShaderPath = "textured2d.frag.spv",
                       VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    [[nodiscard]] VkDescriptorSet CreateTextureDescriptorSet(VkImageView imageView, VkSampler sampler);
    void UpdateTextureDescriptorSet(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler);
    void Destroy();

    [[nodiscard]] VkPipeline Get() const noexcept { return pipeline_; }
    [[nodiscard]] VkPipelineLayout Layout() const noexcept { return layout_; }
    [[nodiscard]] VkPipeline Textured() const noexcept { return texturedPipeline_; }
    [[nodiscard]] VkPipelineLayout TexturedLayout() const noexcept { return texturedLayout_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureDescriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool textureDescriptorPool_ = VK_NULL_HANDLE;
    VkPipelineLayout texturedLayout_ = VK_NULL_HANDLE;
    VkPipeline texturedPipeline_ = VK_NULL_HANDLE;
};

} // namespace wbwwb::vulkan
