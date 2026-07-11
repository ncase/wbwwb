#pragma once

#include "AudioBackend.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace wb {

class AudioManager {
public:
    explicit AudioManager(std::unique_ptr<AudioBackend> backend);
    ~AudioManager();

    void shutdown();
    void preloadSound(const std::string& relativePath);
    void preloadMusic(const std::string& relativePath);
    void playSound(const std::string& relativePath, float volume = 1.0f);
    void playMusic(const std::string& relativePath, float volume = 1.0f, bool loop = true);
    void stopMusic(const std::string& relativePath);
    void stopAllMusic();
    void fadeMusic(const std::string& relativePath, float from, float to, float seconds);
    void setVolumeMix(float master, float music, float sfx);
    void update(float dt);

private:
    float effectiveSoundVolume(float volume) const;
    float effectiveMusicVolume(float volume) const;

    struct MusicState {
        render::MusicHandle music{};
        float volume = 1.0f;
        float fadeFrom = 1.0f;
        float fadeTo = 1.0f;
        float fadeTime = 0.0f;
        float fadeDuration = 0.0f;
        bool fading = false;
        bool playing = false;
    };

    std::unique_ptr<AudioBackend> backend_;
    std::unordered_map<std::string, MusicState> music_;
    float masterVolume_ = 1.0f;
    float musicVolume_ = 1.0f;
    float sfxVolume_ = 1.0f;
};

}  // namespace wb
