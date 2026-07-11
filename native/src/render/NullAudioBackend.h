#pragma once

#include "AudioBackend.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace wb {

class NullAudioBackend final : public AudioBackend {
public:
    render::SoundHandle loadSound(const std::string& relativePath) override;
    render::MusicHandle loadMusic(const std::string& relativePath) override;
    void playSound(render::SoundHandle sound, float volume) override;
    void playMusic(render::MusicHandle music, float volume, bool loop) override;
    void stopMusic(render::MusicHandle music) override;
    void setMusicVolume(render::MusicHandle music, float volume) override;
    void updateMusic(render::MusicHandle music) override;

private:
    std::unordered_map<std::string, render::SoundHandle> soundHandles_;
    std::unordered_map<std::string, render::MusicHandle> musicHandles_;
    std::uint32_t nextSoundId_ = 1;
    std::uint32_t nextMusicId_ = 1;
};

}  // namespace wb
