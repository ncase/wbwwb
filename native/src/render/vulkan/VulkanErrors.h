#pragma once

// Vulkan error helpers.

#include <vulkan/vulkan.h>

#include <string>

namespace wbwwb::vulkan {

[[nodiscard]] const char* VkResultToString(VkResult result) noexcept;
[[nodiscard]] std::string MakeVkErrorMessage(VkResult result, const char* operation);
void CheckVk(VkResult result, const char* operation);

} // namespace wbwwb::vulkan
