#pragma once

// Vulkan SPIR-V shader-module ownership.

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace wbwwb::vulkan {

class VulkanShaderModule final {
public:
    void Create(VkDevice device, const std::vector<std::uint32_t>& spirvWords);
    void Destroy();

    [[nodiscard]] VkShaderModule Get() const noexcept { return module_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkShaderModule module_ = VK_NULL_HANDLE;
};

} // namespace wbwwb::vulkan
