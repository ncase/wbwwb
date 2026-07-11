#pragma once

#include "AudioBackend.h"

#include "raylib.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace wb {

class RaylibAudioBackend final : public AudioBackend {
public:
    explicit RaylibAudioBackend(std::filesystem::path assetRoot);
    ~RaylibAudioBackend() override;

    render::SoundHandle loadSound(const std::string& relativePath) override;
    render::MusicHandle loadMusic(const std::string& relativePath) override;
    void playSound(render::SoundHandle sound, float volume) override;
    void playMusic(render::MusicHandle music, float volume, bool loop) override;
    void stopMusic(render::MusicHandle music) override;
    void setMusicVolume(render::MusicHandle music, float volume) override;
    void updateMusic(render::MusicHandle music) override;

private:
    void ensureAudioDevice();
    std::string path(const std::string& relativePath) const;

    std::filesystem::path assetRoot_;
    std::unordered_map<std::string, render::SoundHandle> soundHandles_;
    std::unordered_map<std::string, render::MusicHandle> musicHandles_;
    std::unordered_map<std::uint32_t, ::Sound> sounds_;
    std::unordered_map<std::uint32_t, ::Music> music_;
    std::uint32_t nextSoundId_ = 1;
    std::uint32_t nextMusicId_ = 1;
    bool ownsAudioDevice_ = false;
};

}  // namespace wb
