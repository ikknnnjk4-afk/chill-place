#include "AudioManager.h"
#include "raylib.h"

void AudioManager::Init() {
    InitAudioDevice();
    ready = true;
}

void AudioManager::PlayAmbient() {
    if (!ready) return;
    // Ambient music (if asset exists — graceful fallback if not)
    // ambient = LoadMusicStream("sounds/ambient.ogg");
    // if (ambient.stream.sampleRate > 0) {
    //     SetMusicVolume(ambient, 0.4f);
    //     PlayMusicStream(ambient);
    // }
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
