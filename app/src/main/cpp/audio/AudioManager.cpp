#include "AudioManager.h"
#include "raylib.h"
#include <android/log.h>

#define LOG_TAG "ChillPlace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void AudioManager::Init() {
    // InitAudioDevice can crash on some Xiaomi devices if OpenSL fails.
    // Wrap with a try-ish approach: just call it and mark ready only if it seems OK.
    LOGI("AudioManager::Init");
    InitAudioDevice();
    // raylib doesn't give a clear success flag, assume OK if we reach here
    ready = true;
    LOGI("AudioManager ready");
}

void AudioManager::PlayAmbient() {
    if (!ready) return;
    // Ambient music disabled for now (no asset) – keeps startup safe
}

void AudioManager::Shutdown() {
    if (ready) {
        if (ambient.stream.sampleRate > 0) {
            StopMusicStream(ambient);
            UnloadMusicStream(ambient);
        }
        CloseAudioDevice();
        ready = false;
    }
}
