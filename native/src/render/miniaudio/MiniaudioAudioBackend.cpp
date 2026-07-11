#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "render/miniaudio/MiniaudioAudioBackend.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

namespace wb {

struct MiniaudioAudioBackend::Impl {
    struct Slot {
        ma_sound sound{};
        bool initialized = false;
    };

    explicit Impl(std::filesystem::path root) : assetRoot(std::move(root)) {
        const ma_result result = ma_engine_init(nullptr, &engine);
        ready = result == MA_SUCCESS;
        if (!ready) {
            std::cerr << "Warning: miniaudio could not initialize an output device: "
                      << ma_result_description(result) << ". Vulkan audio will remain silent.\n";
        }
    }

    ~Impl() {
        for (auto& [_, slot] : sounds) {
            if (slot && slot->initialized) {
                ma_sound_uninit(&slot->sound);
            }
        }
        for (auto& [_, slot] : music) {
            if (slot && slot->initialized) {
                ma_sound_uninit(&slot->sound);
            }
        }
        if (ready) {
            ma_engine_uninit(&engine);
        }
    }

    template <typename Handle>
    Handle load(const std::string& relativePath,
                bool stream,
                std::unordered_map<std::string, Handle>& paths,
                std::unordered_map<std::uint32_t, std::unique_ptr<Slot>>& slots,
                std::uint32_t& nextId) {
        const auto existing = paths.find(relativePath);
        if (existing != paths.end()) {
            return existing->second;
        }

        const Handle handle{nextId++};
        auto slot = std::make_unique<Slot>();
        if (ready) {
            const std::filesystem::path fullPath = assetRoot / relativePath;
            const std::string utf8Path = fullPath.u8string();
            const ma_uint32 flags = stream ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
            const ma_result result = ma_sound_init_from_file(
                &engine,
                utf8Path.c_str(),
                flags,
                nullptr,
                nullptr,
                &slot->sound
            );
            slot->initialized = result == MA_SUCCESS;
            if (!slot->initialized) {
                std::cerr << "Warning: miniaudio could not load " << relativePath << ": "
                          << ma_result_description(result) << '\n';
            }
        }
        paths.emplace(relativePath, handle);
        slots.emplace(handle.id, std::move(slot));
        return handle;
    }

    static Slot* find(std::unordered_map<std::uint32_t, std::unique_ptr<Slot>>& slots, std::uint32_t id) {
        const auto it = slots.find(id);
        return it != slots.end() ? it->second.get() : nullptr;
    }

    std::filesystem::path assetRoot;
    ma_engine engine{};
    bool ready = false;
    std::unordered_map<std::string, render::SoundHandle> soundPaths;
    std::unordered_map<std::string, render::MusicHandle> musicPaths;
    std::unordered_map<std::uint32_t, std::unique_ptr<Slot>> sounds;
    std::unordered_map<std::uint32_t, std::unique_ptr<Slot>> music;
    std::uint32_t nextSoundId = 1;
    std::uint32_t nextMusicId = 1;
};

MiniaudioAudioBackend::MiniaudioAudioBackend(std::filesystem::path assetRoot)
    : impl_(std::make_unique<Impl>(std::move(assetRoot))) {
}

MiniaudioAudioBackend::~MiniaudioAudioBackend() = default;

render::SoundHandle MiniaudioAudioBackend::loadSound(const std::string& relativePath) {
    return impl_->load(relativePath,
                       false,
                       impl_->soundPaths,
                       impl_->sounds,
                       impl_->nextSoundId);
}

render::MusicHandle MiniaudioAudioBackend::loadMusic(const std::string& relativePath) {
    return impl_->load(relativePath,
                       true,
                       impl_->musicPaths,
                       impl_->music,
                       impl_->nextMusicId);
}

void MiniaudioAudioBackend::playSound(render::SoundHandle sound, float volume) {
    Impl::Slot* slot = Impl::find(impl_->sounds, sound.id);
    if (!slot || !slot->initialized) {
        return;
    }
    ma_sound_stop(&slot->sound);
    ma_sound_seek_to_pcm_frame(&slot->sound, 0);
    ma_sound_set_volume(&slot->sound, std::clamp(volume, 0.0f, 1.0f));
    ma_sound_start(&slot->sound);
}

void MiniaudioAudioBackend::playMusic(render::MusicHandle music, float volume, bool loop) {
    Impl::Slot* slot = Impl::find(impl_->music, music.id);
    if (!slot || !slot->initialized) {
        return;
    }
    ma_sound_stop(&slot->sound);
    ma_sound_seek_to_pcm_frame(&slot->sound, 0);
    ma_sound_set_looping(&slot->sound, loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(&slot->sound, std::clamp(volume, 0.0f, 1.0f));
    ma_sound_start(&slot->sound);
}

void MiniaudioAudioBackend::stopMusic(render::MusicHandle music) {
    Impl::Slot* slot = Impl::find(impl_->music, music.id);
    if (slot && slot->initialized) {
        ma_sound_stop(&slot->sound);
    }
}

void MiniaudioAudioBackend::setMusicVolume(render::MusicHandle music, float volume) {
    Impl::Slot* slot = Impl::find(impl_->music, music.id);
    if (slot && slot->initialized) {
        ma_sound_set_volume(&slot->sound, std::clamp(volume, 0.0f, 1.0f));
    }
}

void MiniaudioAudioBackend::updateMusic(render::MusicHandle) {
    // The miniaudio engine streams and mixes on its audio callback thread.
}

}  // namespace wb
