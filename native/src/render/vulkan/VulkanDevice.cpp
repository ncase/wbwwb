#include "VulkanDevice.h"

#include "VulkanDebug.h"
#include "VulkanErrors.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <stdexcept>

namespace wbwwb::vulkan {

const std::vector<const char*>& VulkanDevice::RequiredDeviceExtensions() {
    static const std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    return extensions;
}

void VulkanDevice::Create(VkInstance instance, VkSurfaceKHR surface, bool validationEnabled) {
    PickPhysicalDevice(instance, surface);
    queueFamilies_ = FindQueueFamilies(physicalDevice_, surface);
    CreateLogicalDevice(validationEnabled);
}

void VulkanDevice::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    graphicsQueue_ = VK_NULL_HANDLE;
    presentQueue_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    queueFamilies_ = {};
    hdrMetadataEnabled_ = false;
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        CheckVk(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport),
                "checking Vulkan present support");
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.IsComplete()) {
            break;
        }
    }

    return indices;
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t extensionCount = 0;
    CheckVk(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr),
            "enumerating Vulkan device extension count");

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    CheckVk(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()),
            "enumerating Vulkan device extensions");

    std::set<std::string> required(RequiredDeviceExtensions().begin(), RequiredDeviceExtensions().end());
    for (const auto& extension : availableExtensions) {
        required.erase(extension.extensionName);
    }
    return required.empty();
}

SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport(VkSurfaceKHR surface) const {
    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot query swapchain support before physical device selection");
    }

    SwapchainSupportDetails details;
    CheckVk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface, &details.capabilities),
            "querying Vulkan surface capabilities");

    uint32_t formatCount = 0;
    CheckVk(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface, &formatCount, nullptr),
            "querying Vulkan surface format count");
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        CheckVk(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface, &formatCount, details.formats.data()),
                "querying Vulkan surface formats");
    }

    uint32_t presentModeCount = 0;
    CheckVk(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, nullptr),
            "querying Vulkan present mode count");
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        CheckVk(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, details.presentModes.data()),
                "querying Vulkan present modes");
    }

    return details;
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    QueueFamilyIndices indices = FindQueueFamilies(device, surface);
    if (!indices.IsComplete()) {
        return false;
    }

    if (!CheckDeviceExtensionSupport(device)) {
        return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    SwapchainSupportDetails swapchainSupport;
    VkPhysicalDevice previous = physicalDevice_;
    const_cast<VulkanDevice*>(this)->physicalDevice_ = device;
    swapchainSupport = QuerySwapchainSupport(surface);
    const_cast<VulkanDevice*>(this)->physicalDevice_ = previous;

    return !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
}

void VulkanDevice::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    CheckVk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
            "enumerating Vulkan physical device count");

    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan-capable GPU was found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    CheckVk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
            "enumerating Vulkan physical devices");

    // Prefer discrete GPUs but accept integrated GPUs if suitable.
    VkPhysicalDevice fallback = VK_NULL_HANDLE;
    for (const auto& device : devices) {
        if (!IsDeviceSuitable(device, surface)) {
            continue;
        }

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physicalDevice_ = device;
            return;
        }
        if (fallback == VK_NULL_HANDLE) {
            fallback = device;
        }
    }

    if (fallback != VK_NULL_HANDLE) {
        physicalDevice_ = fallback;
        return;
    }

    throw std::runtime_error("No suitable Vulkan GPU found. Required: graphics queue, present queue, and swapchain support.");
}

void VulkanDevice::CreateLogicalDevice(bool validationEnabled) {
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilies_.graphicsFamily.value(),
        queueFamilies_.presentFamily.value()
    };

    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> deviceExtensions = RequiredDeviceExtensions();
    uint32_t extensionCount = 0;
    CheckVk(vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionCount, nullptr),
            "enumerating optional Vulkan device extension count");
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    CheckVk(vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionCount, availableExtensions.data()),
            "enumerating optional Vulkan device extensions");
    hdrMetadataEnabled_ = std::any_of(availableExtensions.begin(), availableExtensions.end(), [](const VkExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, VK_EXT_HDR_METADATA_EXTENSION_NAME) == 0;
    });
    if (hdrMetadataEnabled_) {
        deviceExtensions.push_back(VK_EXT_HDR_METADATA_EXTENSION_NAME);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (validationEnabled) {
        const auto& layers = VulkanDebug::ValidationLayers();
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    CheckVk(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_),
            "creating Vulkan logical device");

    vkGetDeviceQueue(device_, queueFamilies_.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilies_.presentFamily.value(), 0, &presentQueue_);
}

std::uint32_t VulkanDevice::FindMemoryType(std::uint32_t typeFilter,
                                           VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);

    for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find a suitable Vulkan memory type");
}

} // namespace wbwwb::vulkan
