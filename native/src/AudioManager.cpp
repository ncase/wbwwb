#include "AudioManager.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace wb {

AudioManager::AudioManager(std::unique_ptr<AudioBackend> backend)
    : backend_(std::move(backend)) {
    if (!backend_) {
        throw std::invalid_argument("AudioManager requires an AudioBackend");
    }
}

AudioManager::~AudioManager() = default;

void AudioManager::shutdown() {
    if (backend_) {
        stopAllMusic();
    }
    music_.clear();
    backend_.reset();
}

void AudioManager::preloadSound(const std::string& relativePath) {
    backend_->loadSound(relativePath);
}

void AudioManager::preloadMusic(const std::string& relativePath) {
    backend_->loadMusic(relativePath);
}

void AudioManager::playSound(const std::string& relativePath, float volume) {
    const render::SoundHandle sound = backend_->loadSound(relativePath);
    backend_->playSound(sound, effectiveSoundVolume(volume));
}

void AudioManager::playMusic(const std::string& relativePath, float volume, bool loop) {
    const render::MusicHandle music = backend_->loadMusic(relativePath);

    auto& state = music_[relativePath];
    state.music = music;
    state.volume = volume;
    state.fadeFrom = volume;
    state.fadeTo = volume;
    state.fadeTime = 0.0f;
    state.fadeDuration = 0.0f;
    state.fading = false;
    state.playing = true;

    backend_->playMusic(music, effectiveMusicVolume(volume), loop);
}

void AudioManager::stopMusic(const std::string& relativePath) {
    auto it = music_.find(relativePath);
    if (it == music_.end() || it->second.music.id == 0) {
        return;
    }
    backend_->stopMusic(it->second.music);
    it->second.playing = false;
}

void AudioManager::stopAllMusic() {
    for (auto& [_, state] : music_) {
        if (state.music.id != 0 && state.playing) {
            backend_->stopMusic(state.music);
            state.playing = false;
        }
    }
}

void AudioManager::fadeMusic(const std::string& relativePath, float from, float to, float seconds) {
    auto it = music_.find(relativePath);
    if (it == music_.end() || it->second.music.id == 0) {
        playMusic(relativePath, from, true);
        it = music_.find(relativePath);
    }

    auto& state = it->second;
    state.fadeFrom = from;
    state.fadeTo = to;
    state.fadeTime = 0.0f;
    state.fadeDuration = std::max(seconds, 0.001f);
    state.fading = true;
    state.volume = from;
    backend_->setMusicVolume(state.music, effectiveMusicVolume(from));
}

void AudioManager::setVolumeMix(float master, float music, float sfx) {
    masterVolume_ = std::clamp(master, 0.0f, 1.0f);
    musicVolume_ = std::clamp(music, 0.0f, 1.0f);
    sfxVolume_ = std::clamp(sfx, 0.0f, 1.0f);

    for (auto& [_, state] : music_) {
        if (state.music.id != 0 && state.playing) {
            backend_->setMusicVolume(state.music, effectiveMusicVolume(state.volume));
        }
    }
}

void AudioManager::update(float dt) {
    for (auto& [_, state] : music_) {
        if (state.music.id == 0 || !state.playing) {
            continue;
        }

        if (state.fading) {
            state.fadeTime += dt;
            const float t = std::clamp(state.fadeTime / state.fadeDuration, 0.0f, 1.0f);
            state.volume = state.fadeFrom + (state.fadeTo - state.fadeFrom) * t;
            backend_->setMusicVolume(state.music, effectiveMusicVolume(state.volume));
            if (t >= 1.0f) {
                state.fading = false;
            }
        }

        backend_->updateMusic(state.music);
    }
}

float AudioManager::effectiveSoundVolume(float volume) const {
    return std::clamp(volume, 0.0f, 1.0f) * masterVolume_ * sfxVolume_;
}

float AudioManager::effectiveMusicVolume(float volume) const {
    return std::clamp(volume, 0.0f, 1.0f) * masterVolume_ * musicVolume_;
}

}  // namespace wb
