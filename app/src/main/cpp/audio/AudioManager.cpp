#include "AudioManager.h"
#include "raylib.h"
#include <android/log.h>

#define LOG_TAG "ChillPlace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void AudioManager::Init() {
    LOGI("AudioManager::Init – calling InitAudioDevice");
    // InitAudioDevice can crash on some HyperOS builds if OpenSL fails.
    // We still call it (needed for future sounds) but isolate the risk
    // by doing it after the window is already up and menu is loaded.
    InitAudioDevice();
    ready = true; // assume OK after InitAudioDevice (raylib has no reliable fail flag on all builds)
    LOGI("AudioManager ready=%d", (int)ready);
}

void AudioManager::PlayAmbient() {
    // No ambient file shipped – intentionally empty (game content unchanged)
    (void)ready;
}

void AudioManager::Shutdown() {
    if (!ready) return;
    if (ambient.stream.sampleRate > 0) {
        StopMusicStream(ambient);
        UnloadMusicStream(ambient);
        ambient = Music{};
    }
    CloseAudioDevice();
    ready = false;
}
