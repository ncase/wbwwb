# Native WBWWB C++ Port Vulkan backend CMake fragment.
# Include this from native/CMakeLists.txt when WBWWB_RENDER_BACKEND=vulkan.

if(CMAKE_SIZEOF_VOID_P LESS 8)
    message(FATAL_ERROR "WBWWB_RENDER_BACKEND=vulkan is 64-bit only. Use WBWWB_RENDER_BACKEND=raylib for 32-bit builds.")
endif()

if(NOT TARGET Vulkan::Vulkan)
    find_package(Vulkan REQUIRED)
endif()

get_filename_component(WBWWB_NATIVE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(WBWWB_VULKAN_BACKEND_SOURCES
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanBackend.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanInstance.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanDebug.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanSurface.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanDevice.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanSwapchain.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanCommandContext.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanSync.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanRenderPass.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanFramebuffers.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanFont.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanPipeline.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanShaderModule.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanImageLoader.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanTexture.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanAppWindow.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanCaptureService.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanFramePipeline.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanInputProvider.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanRenderer.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanRenderer2D.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanRenderTarget.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanHDR.cpp"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanErrors.cpp"
)

set(WBWWB_VULKAN_BACKEND_HEADERS
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanBackend.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanInstance.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanDebug.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanSurface.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanDevice.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanSwapchain.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanCommandContext.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanSync.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanRenderPass.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanFramebuffers.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanFont.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanPipeline.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanShaderModule.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanImageLoader.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanTexture.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanAppWindow.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanCaptureService.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanFramePipeline.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanInputProvider.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanRenderer.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanRenderer2D.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanRenderTarget.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanHDR.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanErrors.h"
    "${WBWWB_NATIVE_ROOT}/src/render/vulkan/VulkanTypes.h"
)

# Example usage after your native executable/library target exists:
# target_sources(wbwwb_native PRIVATE ${WBWWB_VULKAN_BACKEND_SOURCES} ${WBWWB_VULKAN_BACKEND_HEADERS})
# target_link_libraries(wbwwb_native PRIVATE Vulkan::Vulkan)
# target_compile_definitions(wbwwb_native PRIVATE WBWWB_RENDER_BACKEND_VULKAN=1)
# target_include_directories(wbwwb_native PRIVATE "${WBWWB_NATIVE_ROOT}/src")
