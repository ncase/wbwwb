#include "VulkanInstance.h"

#include "VulkanErrors.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace wbwwb::vulkan {

bool VulkanInstance::IsExtensionAvailable(const char* extensionName) {
    uint32_t extensionCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS) {
        return false;
    }

    return std::any_of(availableExtensions.begin(), availableExtensions.end(), [extensionName](const auto& ext) {
        return std::strcmp(ext.extensionName, extensionName) == 0;
    });
}

std::vector<const char*> VulkanInstance::BuildExtensionList(
    bool validationEnabled,
    const std::vector<const char*>& requiredExtensions) {
    std::vector<const char*> extensions = requiredExtensions;

    if (validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    std::sort(extensions.begin(), extensions.end(), [](const char* a, const char* b) {
        return std::strcmp(a, b) < 0;
    });
    extensions.erase(std::unique(extensions.begin(), extensions.end(), [](const char* a, const char* b) {
        return std::strcmp(a, b) == 0;
    }), extensions.end());

    return extensions;
}

void VulkanInstance::CheckRequiredExtensions(const std::vector<const char*>& requiredExtensions) {
    uint32_t extensionCount = 0;
    CheckVk(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr),
            "enumerating Vulkan instance extension count");

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    CheckVk(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()),
            "enumerating Vulkan instance extensions");

    for (const char* required : requiredExtensions) {
        bool found = false;
        for (const auto& ext : availableExtensions) {
            if (std::strcmp(required, ext.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(std::string("Required Vulkan instance extension is unavailable: ") + required);
        }
    }
}

void VulkanInstance::Create(const std::string& applicationName,
                            bool enableValidation,
                            const std::vector<const char*>& requiredExtensions) {
#ifndef NDEBUG
    validationEnabled_ = enableValidation && VulkanDebug::CheckValidationLayerSupport();
#else
    (void)enableValidation;
    validationEnabled_ = false;
#endif

    const auto extensions = BuildExtensionList(validationEnabled_, requiredExtensions);
    CheckRequiredExtensions(extensions);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Native WBWWB C++ Port";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (validationEnabled_) {
        const auto& layers = VulkanDebug::ValidationLayers();
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
        VulkanDebug::PopulateCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    CheckVk(vkCreateInstance(&createInfo, nullptr, &instance_), "creating Vulkan instance");
    debug_.Create(instance_, validationEnabled_);
}

void VulkanInstance::Destroy() {
    debug_.Destroy();
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    validationEnabled_ = false;
}

} // namespace wbwwb::vulkan
