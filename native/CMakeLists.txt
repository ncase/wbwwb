cmake_minimum_required(VERSION 3.20)

project(wbwwb_native VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
  enable_language(RC)
endif()

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

set(WBWWB_RENDER_BACKEND "raylib" CACHE STRING "Native render backend: raylib or vulkan")
set_property(CACHE WBWWB_RENDER_BACKEND PROPERTY STRINGS raylib vulkan)
string(TOLOWER "${WBWWB_RENDER_BACKEND}" WBWWB_RENDER_BACKEND_NORMALIZED)

if(NOT (WBWWB_RENDER_BACKEND_NORMALIZED STREQUAL "raylib") AND
   NOT (WBWWB_RENDER_BACKEND_NORMALIZED STREQUAL "vulkan"))
  message(FATAL_ERROR "Unsupported WBWWB_RENDER_BACKEND='${WBWWB_RENDER_BACKEND}'. Use 'raylib' or 'vulkan'.")
endif()

if(WBWWB_RENDER_BACKEND_NORMALIZED STREQUAL "vulkan")
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "The Vulkan/HDR backend is 64-bit only. Use -DWBWWB_RENDER_BACKEND=raylib for 32-bit builds.")
  endif()
  if(NOT WIN32)
    message(FATAL_ERROR
      "WBWWB_RENDER_BACKEND=vulkan currently supports Windows x64 only. "
      "Use -DWBWWB_RENDER_BACKEND=raylib for Linux builds until a native Linux Vulkan surface/window path is added.")
  endif()
  find_package(Vulkan QUIET COMPONENTS glslc)
  if(NOT Vulkan_FOUND)
    message(FATAL_ERROR
      "WBWWB_RENDER_BACKEND=vulkan requires the Vulkan SDK or Vulkan development packages. "
      "Install Vulkan headers/libraries and make sure CMake can find them through VULKAN_SDK or the system package paths.")
  endif()
  if(NOT Vulkan_GLSLC_EXECUTABLE)
    message(FATAL_ERROR
      "WBWWB_RENDER_BACKEND=vulkan requires glslc for Vulkan shader compilation. "
      "Install the Vulkan SDK shader tools or add glslc to PATH.")
  endif()
  message(STATUS "Vulkan SDK detected: ${Vulkan_INCLUDE_DIRS}")
  message(STATUS "Vulkan library: ${Vulkan_LIBRARIES}")
  message(STATUS "Vulkan shader compiler: ${Vulkan_GLSLC_EXECUTABLE}")
endif()

if(WBWWB_RENDER_BACKEND_NORMALIZED STREQUAL "raylib")
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
  )

  FetchContent_MakeAvailable(raylib)
  set_property(DIRECTORY "${raylib_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL YES)
  FetchContent_MakeAvailable(nlohmann_json)

  message(STATUS "Native WBWWB C++ Port renderer: raylib/OpenGL")

  add_executable(wbwwb_native
    src/AssetManager.cpp
    src/AudioManager.cpp
    src/compat/RaylibCompat.cpp
    src/Game.cpp
    src/Gameplay.cpp
    src/Localization.cpp
    src/main.cpp
    src/render/RenderBackend.cpp
    src/render/NullAudioBackend.cpp
    src/render/raylib/RaylibAppWindow.cpp
    src/render/raylib/RaylibAudioBackend.cpp
    src/render/raylib/RaylibCaptureService.cpp
    src/render/raylib/RaylibFramePipeline.cpp
    src/render/raylib/RaylibGame.cpp
    src/render/raylib/RaylibInputProvider.cpp
    src/render/raylib/RaylibRenderer.cpp
    src/Scene.cpp
    src/Scenes.cpp
    src/Settings.cpp
    src/Tween.cpp
    src/WindowIcon.cpp
  )

  target_include_directories(wbwwb_native PRIVATE src resources)
  target_link_libraries(wbwwb_native PRIVATE raylib nlohmann_json::nlohmann_json)
  target_compile_definitions(wbwwb_native PRIVATE
    WBWWB_RENDER_BACKEND_RAYLIB=1
    WBWWB_RENDER_BACKEND_NAME="raylib-opengl"
  )
elseif(WBWWB_RENDER_BACKEND_NORMALIZED STREQUAL "vulkan")
  include("${CMAKE_CURRENT_LIST_DIR}/cmake/WBWWB_VulkanBackend.cmake")
  FetchContent_MakeAvailable(nlohmann_json)
  FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.25
    GIT_SHALLOW TRUE
  )
  set(MINIAUDIO_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(miniaudio)

  FetchContent_Declare(
    fidelityfx_fsr1
    GIT_REPOSITORY https://github.com/GPUOpen-Effects/FidelityFX-FSR.git
    GIT_TAG a21ffb8f6c13233ba336352bdff293894c706575
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
    GIT_SUBMODULES_RECURSE FALSE
  )
  FetchContent_MakeAvailable(fidelityfx_fsr1)
  set(WBWWB_FSR1_INCLUDE_DIR "${fidelityfx_fsr1_SOURCE_DIR}/ffx-fsr")

  message(STATUS "Native WBWWB C++ Port renderer: Vulkan")

  set(WBWWB_VULKAN_SHADER_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/assets/shaders/vulkan")
  set(WBWWB_VULKAN_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/vulkan-shaders")
  file(MAKE_DIRECTORY "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}")
  set(WBWWB_VULKAN_SHADER_OUTPUTS
    "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/solid2d.vert.spv"
    "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/solid2d.frag.spv"
    "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/textured2d.vert.spv"
    "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/textured2d.frag.spv"
    "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/present2d.frag.spv"
    "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/fsr1_easu.frag.spv"
    "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/fsr1_rcas.frag.spv"
  )
  add_custom_command(
    OUTPUT "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/solid2d.vert.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -fshader-stage=vert
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/solid2d.vert"
            -o "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/solid2d.vert.spv"
    DEPENDS "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/solid2d.vert"
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/solid2d.frag.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -fshader-stage=frag
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/solid2d.frag"
            -o "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/solid2d.frag.spv"
    DEPENDS "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/solid2d.frag"
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/textured2d.vert.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -fshader-stage=vert
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/textured2d.vert"
            -o "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/textured2d.vert.spv"
    DEPENDS "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/textured2d.vert"
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/textured2d.frag.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -fshader-stage=frag
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/textured2d.frag"
            -o "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/textured2d.frag.spv"
    DEPENDS "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/textured2d.frag"
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/present2d.frag.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -fshader-stage=frag
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/present2d.frag"
            -o "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/present2d.frag.spv"
    DEPENDS "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/present2d.frag"
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/fsr1_easu.frag.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -I "${WBWWB_FSR1_INCLUDE_DIR}" -fshader-stage=frag
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/fsr1_easu.frag"
            -o "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/fsr1_easu.frag.spv"
    DEPENDS
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/fsr1_easu.frag"
            "${WBWWB_FSR1_INCLUDE_DIR}/ffx_a.h"
            "${WBWWB_FSR1_INCLUDE_DIR}/ffx_fsr1.h"
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/fsr1_rcas.frag.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" -I "${WBWWB_FSR1_INCLUDE_DIR}" -fshader-stage=frag
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/fsr1_rcas.frag"
            -o "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/fsr1_rcas.frag.spv"
    DEPENDS
            "${WBWWB_VULKAN_SHADER_SOURCE_DIR}/fsr1_rcas.frag"
            "${WBWWB_FSR1_INCLUDE_DIR}/ffx_a.h"
            "${WBWWB_FSR1_INCLUDE_DIR}/ffx_fsr1.h"
    VERBATIM
  )
  add_custom_target(wbwwb_vulkan_shaders DEPENDS ${WBWWB_VULKAN_SHADER_OUTPUTS})
  string(REPLACE "\\" "/" WBWWB_VULKAN_SHADER_OUTPUT_DIR_DEFINE "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}")

  add_executable(wbwwb_native
    src/AssetManager.cpp
    src/AudioManager.cpp
    src/compat/RaylibCompat.cpp
    src/Game.cpp
    src/Gameplay.cpp
    src/Localization.cpp
    src/render/NullAudioBackend.cpp
    src/render/miniaudio/MiniaudioAudioBackend.cpp
    src/Scene.cpp
    src/Scenes.cpp
    src/Settings.cpp
    src/Tween.cpp
    src/render/RenderBackend.cpp
    src/render/vulkan/VulkanSmokeMain.cpp
    ${WBWWB_VULKAN_BACKEND_SOURCES}
    ${WBWWB_VULKAN_BACKEND_HEADERS}
  )

  target_include_directories(wbwwb_native PRIVATE src resources "${miniaudio_SOURCE_DIR}")
  target_link_libraries(wbwwb_native PRIVATE Vulkan::Vulkan nlohmann_json::nlohmann_json)
  if(WIN32)
    target_link_libraries(wbwwb_native PRIVATE windowscodecs ole32)
  endif()
  target_compile_definitions(wbwwb_native PRIVATE
    WBWWB_RENDER_BACKEND_VULKAN=1
    WBWWB_RENDER_BACKEND_NAME="vulkan"
    WBWWB_VULKAN_SHADER_DIR="${WBWWB_VULKAN_SHADER_OUTPUT_DIR_DEFINE}"
    VK_USE_PLATFORM_WIN32_KHR
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )
  add_dependencies(wbwwb_native wbwwb_vulkan_shaders)
  add_custom_command(TARGET wbwwb_native POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:wbwwb_native>/vulkan-shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/solid2d.vert.spv"
            "$<TARGET_FILE_DIR:wbwwb_native>/vulkan-shaders/solid2d.vert.spv"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/solid2d.frag.spv"
            "$<TARGET_FILE_DIR:wbwwb_native>/vulkan-shaders/solid2d.frag.spv"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/textured2d.vert.spv"
            "$<TARGET_FILE_DIR:wbwwb_native>/vulkan-shaders/textured2d.vert.spv"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/textured2d.frag.spv"
            "$<TARGET_FILE_DIR:wbwwb_native>/vulkan-shaders/textured2d.frag.spv"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/present2d.frag.spv"
            "$<TARGET_FILE_DIR:wbwwb_native>/vulkan-shaders/present2d.frag.spv"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/fsr1_easu.frag.spv"
            "$<TARGET_FILE_DIR:wbwwb_native>/vulkan-shaders/fsr1_easu.frag.spv"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WBWWB_VULKAN_SHADER_OUTPUT_DIR}/fsr1_rcas.frag.spv"
            "$<TARGET_FILE_DIR:wbwwb_native>/vulkan-shaders/fsr1_rcas.frag.spv"
    VERBATIM
  )
endif()

if(WIN32)
  target_sources(wbwwb_native PRIVATE resources/wbwwb.rc)
endif()

if(WIN32)
  target_link_libraries(wbwwb_native PRIVATE user32 gdi32)
endif()

if(MSVC)
  target_compile_options(wbwwb_native PRIVATE /W4)
else()
  target_compile_options(wbwwb_native PRIVATE -Wall -Wextra -Wpedantic)
endif()

function(wbwwb_package_license source_file output_name)
  if(NOT EXISTS "${source_file}")
    message(FATAL_ERROR "Missing third-party license: ${source_file}")
  endif()
  add_custom_command(TARGET wbwwb_native POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:wbwwb_native>/licenses"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${source_file}"
            "$<TARGET_FILE_DIR:wbwwb_native>/licenses/${output_name}"
    VERBATIM
  )
  install(FILES "${source_file}"
    DESTINATION share/licenses/wbwwb
    RENAME "${output_name}"
  )
endfunction()

wbwwb_package_license(
  "${CMAKE_CURRENT_LIST_DIR}/THIRD_PARTY_NOTICES.md"
  "THIRD_PARTY_NOTICES.md"
)
wbwwb_package_license(
  "${nlohmann_json_SOURCE_DIR}/LICENSE.MIT"
  "nlohmann-json.txt"
)
if(WBWWB_RENDER_BACKEND_NORMALIZED STREQUAL "raylib")
  wbwwb_package_license("${raylib_SOURCE_DIR}/LICENSE" "raylib.txt")
  wbwwb_package_license(
    "${raylib_SOURCE_DIR}/src/external/glfw/LICENSE.md"
    "GLFW.txt"
  )
else()
  wbwwb_package_license("${miniaudio_SOURCE_DIR}/LICENSE" "miniaudio.txt")
  wbwwb_package_license(
    "${fidelityfx_fsr1_SOURCE_DIR}/license.txt"
    "FidelityFX-FSR.txt"
  )
endif()

install(TARGETS wbwwb_native RUNTIME DESTINATION bin)
install(DIRECTORY
  "${CMAKE_CURRENT_LIST_DIR}/../css"
  "${CMAKE_CURRENT_LIST_DIR}/../sounds"
  "${CMAKE_CURRENT_LIST_DIR}/../sprites"
  DESTINATION share/wbwwb
  PATTERN "Thumbs.db" EXCLUDE
  PATTERN ".DS_Store" EXCLUDE
)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/assets"
  DESTINATION share/wbwwb/native
)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../translations"
  DESTINATION bin
)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/../sprites/app_icon.png"
  DESTINATION share/icons/hicolor/256x256/apps
  RENAME wbwwb_native.png
)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/resources/wbwwb_native.desktop"
  DESTINATION share/applications
)
if(WBWWB_RENDER_BACKEND_NORMALIZED STREQUAL "vulkan")
  install(FILES ${WBWWB_VULKAN_SHADER_OUTPUTS}
    DESTINATION bin/vulkan-shaders
  )
endif()
