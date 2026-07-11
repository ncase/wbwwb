#pragma once

#include "render/Renderer.h"

#include <string>

namespace wb {

class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    virtual render::SoundHandle loadSound(const std::string& relativePath) = 0;
    virtual render::MusicHandle loadMusic(const std::string& relativePath) = 0;
    virtual void playSound(render::SoundHandle sound, float volume) = 0;
    virtual void playMusic(render::MusicHandle music, float volume, bool loop) = 0;
    virtual void stopMusic(render::MusicHandle music) = 0;
    virtual void setMusicVolume(render::MusicHandle music, float volume) = 0;
    virtual void updateMusic(render::MusicHandle music) = 0;
};

}  // namespace wb
