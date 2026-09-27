#pragma once

#include <string>

// Forward declarations to avoid exposing miniaudio in the header
struct ma_engine;
struct ma_sound;

namespace ols::core {

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool initialize();
    void playAlertSound();

private:
    ma_engine* engine_{nullptr};
    ma_sound*  alert_sound_{nullptr};
    bool initialized_{false};
};

} // namespace ols::core
