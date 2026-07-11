#include "render/NullAudioBackend.h"

namespace wb {

render::SoundHandle NullAudioBackend::loadSound(const std::string& relativePath) {
    auto it = soundHandles_.find(relativePath);
    if (it != soundHandles_.end()) {
        return it->second;
    }
    render::SoundHandle handle{nextSoundId_++};
    soundHandles_[relativePath] = handle;
    return handle;
}

render::MusicHandle NullAudioBackend::loadMusic(const std::string& relativePath) {
    auto it = musicHandles_.find(relativePath);
    if (it != musicHandles_.end()) {
        return it->second;
    }
    render::MusicHandle handle{nextMusicId_++};
    musicHandles_[relativePath] = handle;
    return handle;
}

void NullAudioBackend::playSound(render::SoundHandle, float) {}
void NullAudioBackend::playMusic(render::MusicHandle, float, bool) {}
void NullAudioBackend::stopMusic(render::MusicHandle) {}
void NullAudioBackend::setMusicVolume(render::MusicHandle, float) {}
void NullAudioBackend::updateMusic(render::MusicHandle) {}

}  // namespace wb
