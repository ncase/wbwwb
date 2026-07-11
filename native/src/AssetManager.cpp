#include "AssetManager.h"

#include "Constants.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace wb {
namespace {

render::Rect rectFromJson(const nlohmann::json& value) {
    return render::Rect{
        value.value("x", 0.0f),
        value.value("y", 0.0f),
        value.value("w", 0.0f),
        value.value("h", 0.0f)
    };
}

render::Vec2 sourceSizeFromJson(const nlohmann::json& value) {
    return render::Vec2{value.value("w", 0.0f), value.value("h", 0.0f)};
}

float atlasScaleFromJson(const nlohmann::json& meta) {
    const auto scaleValue = meta.find("scale");
    if (scaleValue == meta.end()) {
        return 1.0f;
    }

    try {
        const float scale = scaleValue->is_string()
            ? std::stof(scaleValue->get<std::string>())
            : scaleValue->get<float>();
        return std::isfinite(scale) && scale > 0.0f ? scale : 1.0f;
    } catch (...) {
        return 1.0f;
    }
}

bool hasAssetFolders(const std::filesystem::path& root) {
    return std::filesystem::exists(root / "sprites") &&
           std::filesystem::exists(root / "sounds");
}

bool isResolutionMatchedPicture(const std::string& relativePath) {
    static const std::array<std::string_view, 14> pictures{{
        "sprites/bg_preload.png",
        "sprites/bg_preload_2.png",
        "sprites/quote/quote0001.png",
        "sprites/quote/quote0002.png",
        "sprites/quote/quote0003.png",
        "sprites/quote/quote0004.png",
        "sprites/credits/credits0001.png",
        "sprites/credits/credits0002.png",
        "sprites/credits/credits0003.png",
        "sprites/credits/credits0004.png",
        "sprites/credits/credits0005.png",
        "sprites/credits/credits0006.png",
        "sprites/credits/credits0007.png",
        "sprites/credits/credits0008.png"
    }};
    return std::find(pictures.begin(), pictures.end(), relativePath) != pictures.end();
}

std::filesystem::path installedAssetRoot(const std::filesystem::path& base) {
    return base / "share" / "wbwwb";
}

std::array<std::filesystem::path, 6> fontCandidates() {
    return {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf"
    };
}

}  // namespace

Atlas::Atlas(render::TextureHandle textureHandle, std::filesystem::path imagePath)
    : textureHandle_(textureHandle), imagePath_(std::move(imagePath)) {}

void Atlas::addFrame(std::string name, AtlasFrame frame) {
    if (frame.rotated) {
        throw std::runtime_error("Rotated atlas frames are not supported: " + name);
    }
    orderedNames_.push_back(name);
    frames_[std::move(name)] = frame;
}

const AtlasFrame& Atlas::frame(const std::string& name) const {
    auto it = frames_.find(name);
    if (it == frames_.end()) {
        throw std::runtime_error("Missing atlas frame: " + name);
    }
    return it->second;
}

const AtlasFrame& Atlas::frameAt(std::size_t index) const {
    if (index >= orderedNames_.size()) {
        throw std::runtime_error("Atlas frame index out of range");
    }
    return frame(orderedNames_[index]);
}

std::vector<std::string> Atlas::framesByPrefix(const std::string& prefix) const {
    std::vector<std::string> result;
    for (const auto& name : orderedNames_) {
        if (name.rfind(prefix, 0) == 0) {
            result.push_back(name);
        }
    }
    return result;
}

std::size_t Atlas::frameCount() const {
    return orderedNames_.size();
}

AssetManager::AssetManager(std::filesystem::path root) : root_(std::move(root)) {}

AssetManager::~AssetManager() = default;

std::filesystem::path AssetManager::findAssetRoot(const char* argv0) {
#ifdef WBWWB_ASSET_ROOT
    std::filesystem::path configuredRoot = WBWWB_ASSET_ROOT;
    if (hasAssetFolders(configuredRoot)) {
        return configuredRoot;
    }
#endif

    std::filesystem::path current = std::filesystem::current_path();
    for (int i = 0; i < 8; ++i) {
        if (hasAssetFolders(current)) {
            return current;
        }
        const std::filesystem::path installedRoot = installedAssetRoot(current);
        if (hasAssetFolders(installedRoot)) {
            return installedRoot;
        }
        if (!current.has_parent_path()) {
            break;
        }
        current = current.parent_path();
    }

    std::filesystem::path executable = std::filesystem::absolute(argv0).parent_path();
    for (int i = 0; i < 8; ++i) {
        if (hasAssetFolders(executable)) {
            return executable;
        }
        const std::filesystem::path installedRoot = installedAssetRoot(executable);
        if (hasAssetFolders(installedRoot)) {
            return installedRoot;
        }
        if (!executable.has_parent_path()) {
            break;
        }
        executable = executable.parent_path();
    }

    throw std::runtime_error("Could not locate sprites/ and sounds/ asset folders.");
}

std::string AssetManager::path(const std::string& relativePath) const {
    return (root_ / relativePath).string();
}

void AssetManager::setRenderer(render::Renderer* renderer) {
    renderer_ = renderer;
}

void AssetManager::setSmoothTextures(bool enabled) {
    if (smoothTextures_ != enabled) {
        resolutionMatchedPicturesDirty_ = true;
    }
    smoothTextures_ = enabled;
    if (renderer_) {
        const render::TextureFilter filter = smoothTextures_ ? render::TextureFilter::Linear : render::TextureFilter::Nearest;
        for (auto& [_, handle] : textureHandles_) {
            renderer_->SetTextureFilter(handle, filter);
        }
    }
}

render::TextureHandle AssetManager::textureHandle(const std::string& relativePath) {
    auto it = textureHandles_.find(relativePath);
    if (it != textureHandles_.end()) {
        return it->second;
    }
    if (!renderer_) {
        throw std::runtime_error("AssetManager has no renderer for texture handle: " + relativePath);
    }
    const bool matchRenderResolution = isResolutionMatchedPicture(relativePath);
    const render::TextureHandle handle = renderer_->LoadTexture(
        path(relativePath),
        smoothTextures_ ? render::TextureFilter::Linear : render::TextureFilter::Nearest,
        matchRenderResolution ? render::TextureSizing::RenderResolution : render::TextureSizing::Source
    );
    if (handle.id == 0) {
        throw std::runtime_error("Failed to load texture handle: " + relativePath);
    }
    textureHandles_[relativePath] = handle;
    if (matchRenderResolution) {
        resolutionMatchedPictures_.insert(relativePath);
    }
    return handle;
}

void AssetManager::releaseTexture(const std::string& relativePath) {
    const auto it = textureHandles_.find(relativePath);
    if (it == textureHandles_.end()) {
        return;
    }
    if (renderer_ != nullptr && it->second.id != 0) {
        renderer_->DestroyTexture(it->second);
    }
    textureHandles_.erase(it);
    resolutionMatchedPictures_.erase(relativePath);
}

void AssetManager::updateRenderResolution(std::uint32_t width, std::uint32_t height) {
    width = std::max(1u, width);
    height = std::max(1u, height);
    const bool resolutionChanged = renderWidth_ != width || renderHeight_ != height;
    if (!resolutionChanged && !resolutionMatchedPicturesDirty_) {
        return;
    }
    renderWidth_ = width;
    renderHeight_ = height;
    if (renderer_ == nullptr) {
        return;
    }
    if (resolutionMatchedPictures_.empty()) {
        resolutionMatchedPicturesDirty_ = false;
        return;
    }

    const render::TextureFilter filter = smoothTextures_ ? render::TextureFilter::Linear : render::TextureFilter::Nearest;
    bool reloadFailed = false;
    for (const std::string& relativePath : resolutionMatchedPictures_) {
        const auto it = textureHandles_.find(relativePath);
        if (it == textureHandles_.end()) {
            continue;
        }
        const render::TextureHandle replacement = renderer_->LoadTexture(
            path(relativePath),
            filter,
            render::TextureSizing::RenderResolution
        );
        if (replacement.id == 0) {
            reloadFailed = true;
            continue;
        }
        const render::TextureHandle previous = it->second;
        it->second = replacement;
        renderer_->DestroyTexture(previous);
    }
    resolutionMatchedPicturesDirty_ = reloadFailed;
}

render::TextureHandle AssetManager::atlasTextureHandle(const std::string& relativeJsonPath) {
    Atlas& value = atlas(relativeJsonPath);
    render::TextureHandle handle = value.textureHandle();
    if (handle.id != 0) {
        return handle;
    }

    handle = textureHandle(value.imagePath().generic_string());
    value.setTextureHandle(handle);
    return handle;
}

render::FontHandle AssetManager::fontHandle(int size) {
    auto it = fontHandles_.find(size);
    if (it != fontHandles_.end()) {
        return it->second;
    }
    if (!renderer_) {
        throw std::runtime_error("AssetManager has no renderer for font handle");
    }
    const render::FontHandle handle = renderer_->LoadFont(fontPathForSize(size), size);
    if (handle.id == 0) {
        throw std::runtime_error("Failed to load font handle");
    }
    fontHandles_[size] = handle;
    return handle;
}

std::filesystem::path AssetManager::fontPathForSize([[maybe_unused]] int size) const {
    for (const auto& fontPath : fontCandidates()) {
        if (std::filesystem::exists(fontPath)) {
            return fontPath;
        }
    }
    return {};
}

Atlas& AssetManager::atlas(const std::string& relativeJsonPath) {
    auto it = atlases_.find(relativeJsonPath);
    if (it != atlases_.end()) {
        return *it->second;
    }

    const std::filesystem::path jsonPath = root_ / relativeJsonPath;
    std::ifstream input(jsonPath);
    if (!input) {
        throw std::runtime_error("Failed to open atlas: " + relativeJsonPath);
    }

    nlohmann::json json;
    input >> json;

    const nlohmann::json& meta = json.at("meta");
    const std::string image = meta.at("image").get<std::string>();
    const float atlasScale = atlasScaleFromJson(meta);
    const std::filesystem::path imageRelative = std::filesystem::path(relativeJsonPath).parent_path() / image;
    render::TextureHandle atlasTextureHandle{};

    auto loadedAtlas = std::make_unique<Atlas>(atlasTextureHandle, imageRelative);
    for (const auto& [name, value] : json.at("frames").items()) {
        AtlasFrame loadedFrame;
        loadedFrame.frame = rectFromJson(value.at("frame"));
        loadedFrame.logicalFrameSize = {
            loadedFrame.frame.width / atlasScale,
            loadedFrame.frame.height / atlasScale
        };
        loadedFrame.rotated = value.value("rotated", false);
        loadedFrame.trimmed = value.value("trimmed", false);
        loadedFrame.spriteSourceSize = rectFromJson(value.at("spriteSourceSize"));
        loadedFrame.sourceSize = sourceSizeFromJson(value.at("sourceSize"));
        loadedFrame.spriteSourceSize.x /= atlasScale;
        loadedFrame.spriteSourceSize.y /= atlasScale;
        loadedFrame.spriteSourceSize.width /= atlasScale;
        loadedFrame.spriteSourceSize.height /= atlasScale;
        loadedFrame.sourceSize.x /= atlasScale;
        loadedFrame.sourceSize.y /= atlasScale;
        loadedAtlas->addFrame(name, loadedFrame);
    }

    Atlas& result = *loadedAtlas;
    atlases_[relativeJsonPath] = std::move(loadedAtlas);
    return result;
}

}  // namespace wb
