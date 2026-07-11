#include "VulkanShaderModule.h"

#include "VulkanErrors.h"

#include <stdexcept>

namespace wbwwb::vulkan {

void VulkanShaderModule::Create(VkDevice device, const std::vector<std::uint32_t>& spirvWords) {
    if (spirvWords.empty()) {
        throw std::runtime_error("Cannot create a Vulkan shader module from empty SPIR-V data");
    }

    device_ = device;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvWords.size() * sizeof(std::uint32_t);
    createInfo.pCode = spirvWords.data();

    CheckVk(vkCreateShaderModule(device_, &createInfo, nullptr, &module_),
            "creating Vulkan shader module");
}

void VulkanShaderModule::Destroy() {
    if (module_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, module_, nullptr);
        module_ = VK_NULL_HANDLE;
    }
    device_ = VK_NULL_HANDLE;
}

} // namespace wbwwb::vulkan
