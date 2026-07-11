#pragma once

#include "AudioBackend.h"

#include <filesystem>
#include <memory>

namespace wb {

class MiniaudioAudioBackend final : public AudioBackend {
public:
    explicit MiniaudioAudioBackend(std::filesystem::path assetRoot);
    ~MiniaudioAudioBackend() override;

    render::SoundHandle loadSound(const std::string& relativePath) override;
    render::MusicHandle loadMusic(const std::string& relativePath) override;
    void playSound(render::SoundHandle sound, float volume) override;
    void playMusic(render::MusicHandle music, float volume, bool loop) override;
    void stopMusic(render::MusicHandle music) override;
    void setMusicVolume(render::MusicHandle music, float volume) override;
    void updateMusic(render::MusicHandle music) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace wb
