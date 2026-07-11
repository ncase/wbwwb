#pragma once

#include "render/Renderer.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace wb {

struct AtlasFrame {
    render::Rect frame{};
    render::Vec2 logicalFrameSize{};
    render::Rect spriteSourceSize{};
    render::Vec2 sourceSize{};
    bool rotated = false;
    bool trimmed = false;
};

class Atlas {
public:
    Atlas() = default;
    Atlas(render::TextureHandle textureHandle, std::filesystem::path imagePath);

    void addFrame(std::string name, AtlasFrame frame);
    const AtlasFrame& frame(const std::string& name) const;
    const AtlasFrame& frameAt(std::size_t index) const;
    std::vector<std::string> framesByPrefix(const std::string& prefix) const;
    std::size_t frameCount() const;
    render::TextureHandle textureHandle() const { return textureHandle_; }
    void setTextureHandle(render::TextureHandle textureHandle) { textureHandle_ = textureHandle; }
    const std::filesystem::path& imagePath() const { return imagePath_; }

private:
    render::TextureHandle textureHandle_{};
    std::filesystem::path imagePath_;
    std::unordered_map<std::string, AtlasFrame> frames_;
    std::vector<std::string> orderedNames_;
};

class AssetManager {
public:
    explicit AssetManager(std::filesystem::path root);
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    static std::filesystem::path findAssetRoot(const char* argv0);

    void setRenderer(render::Renderer* renderer);
    render::TextureHandle textureHandle(const std::string& relativePath);
    void releaseTexture(const std::string& relativePath);
    void updateRenderResolution(std::uint32_t width, std::uint32_t height);
    render::TextureHandle atlasTextureHandle(const std::string& relativeJsonPath);
    render::FontHandle fontHandle(int size);

    Atlas& atlas(const std::string& relativeJsonPath);
    void setSmoothTextures(bool enabled);
    bool smoothTextures() const { return smoothTextures_; }

    std::string path(const std::string& relativePath) const;
    const std::filesystem::path& root() const { return root_; }

private:
    std::filesystem::path fontPathForSize(int size) const;

    std::filesystem::path root_;
    render::Renderer* renderer_ = nullptr;
    std::unordered_map<std::string, render::TextureHandle> textureHandles_;
    std::unordered_set<std::string> resolutionMatchedPictures_;
    std::unordered_map<int, render::FontHandle> fontHandles_;
    std::unordered_map<std::string, std::unique_ptr<Atlas>> atlases_;
    bool smoothTextures_ = false;
    bool resolutionMatchedPicturesDirty_ = false;
    std::uint32_t renderWidth_ = 0;
    std::uint32_t renderHeight_ = 0;
};

}  // namespace wb
