#include <open_lidar_studio/core/audio_manager.hpp>
#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#include <open_lidar_studio/third_party/miniaudio.h>

namespace ols::core {

AudioManager::AudioManager() {
    engine_ = new ma_engine;
    alert_sound_ = new ma_sound;
}

AudioManager::~AudioManager() {
    if (initialized_) {
        ma_sound_uninit(alert_sound_);
        ma_engine_uninit(engine_);
    }
    delete alert_sound_;
    delete engine_;
}

bool AudioManager::initialize() {
    ma_result result = ma_engine_init(NULL, engine_);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio engine." << std::endl;
        return false;
    }

    result = ma_sound_init_from_file(engine_, "assets/sounds/alert.wav", 0, NULL, NULL, alert_sound_);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load alert.wav" << std::endl;
        // Don't fail completely if just the sound is missing
    } else {
        initialized_ = true;
    }
    return true;
}

void AudioManager::playAlertSound() {
    if (initialized_ && alert_sound_) {
        // Restart the sound if it's already playing, or just play it
        if (ma_sound_is_playing(alert_sound_)) {
            ma_sound_seek_to_pcm_frame(alert_sound_, 0);
        } else {
            ma_sound_start(alert_sound_);
        }
    }
}

} // namespace ols::core
