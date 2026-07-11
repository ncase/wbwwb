#include "render/raylib/RaylibAudioBackend.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace wb {

RaylibAudioBackend::RaylibAudioBackend(std::filesystem::path assetRoot)
    : assetRoot_(std::move(assetRoot)) {}

RaylibAudioBackend::~RaylibAudioBackend() {
    for (auto& [_, sound] : sounds_) {
        UnloadSound(sound);
    }
    for (auto& [_, music] : music_) {
        UnloadMusicStream(music);
    }
    if (ownsAudioDevice_) {
        CloseAudioDevice();
    }
}

std::string RaylibAudioBackend::path(const std::string& relativePath) const {
    return (assetRoot_ / relativePath).string();
}

void RaylibAudioBackend::ensureAudioDevice() {
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        ownsAudioDevice_ = true;
    }
}

render::SoundHandle RaylibAudioBackend::loadSound(const std::string& relativePath) {
    auto it = soundHandles_.find(relativePath);
    if (it != soundHandles_.end()) {
        return it->second;
    }

    ensureAudioDevice();
    ::Sound loaded = LoadSound(path(relativePath).c_str());
    if (loaded.frameCount == 0) {
        throw std::runtime_error("Failed to load sound: " + relativePath);
    }

    render::SoundHandle handle{nextSoundId_++};
    soundHandles_[relativePath] = handle;
    sounds_[handle.id] = loaded;
    return handle;
}

render::MusicHandle RaylibAudioBackend::loadMusic(const std::string& relativePath) {
    auto it = musicHandles_.find(relativePath);
    if (it != musicHandles_.end()) {
        return it->second;
    }

    ensureAudioDevice();
    ::Music loaded = LoadMusicStream(path(relativePath).c_str());
    if (loaded.frameCount == 0) {
        throw std::runtime_error("Failed to load music: " + relativePath);
    }

    render::MusicHandle handle{nextMusicId_++};
    musicHandles_[relativePath] = handle;
    music_[handle.id] = loaded;
    return handle;
}

void RaylibAudioBackend::playSound(render::SoundHandle sound, float volume) {
    auto it = sounds_.find(sound.id);
    if (it == sounds_.end()) {
        return;
    }
    ::Sound& value = it->second;
    SetSoundVolume(value, std::clamp(volume, 0.0f, 1.0f));
    PlaySound(value);
}

void RaylibAudioBackend::playMusic(render::MusicHandle music, float volume, bool loop) {
    auto it = music_.find(music.id);
    if (it == music_.end()) {
        return;
    }
    ::Music& value = it->second;
    value.looping = loop;
    SetMusicVolume(value, std::clamp(volume, 0.0f, 1.0f));
    PlayMusicStream(value);
}

void RaylibAudioBackend::stopMusic(render::MusicHandle music) {
    auto it = music_.find(music.id);
    if (it == music_.end()) {
        return;
    }
    StopMusicStream(it->second);
}

void RaylibAudioBackend::setMusicVolume(render::MusicHandle music, float volume) {
    auto it = music_.find(music.id);
    if (it == music_.end()) {
        return;
    }
    SetMusicVolume(it->second, std::clamp(volume, 0.0f, 1.0f));
}

void RaylibAudioBackend::updateMusic(render::MusicHandle music) {
    auto it = music_.find(music.id);
    if (it == music_.end()) {
        return;
    }
    UpdateMusicStream(it->second);
}

}  // namespace wb
