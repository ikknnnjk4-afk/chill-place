#include "AudioManager.h"
#include "raylib.h"
#include <cmath>
#include <vector>

void AudioManager::Init() {
    InitAudioDevice();
    ready = IsAudioDeviceReady();
    // Procedural short click as footstep placeholder (no wav asset yet)
    // We'll synthesize a tiny wave into a Sound via raw data if needed later.
    hasFootstep = false;
}

void AudioManager::PlayAmbient() {
    // Ambient loop would load from assets/audio/ – none provided yet
}

void AudioManager::PlayFootstep() {
    // Soft tick via raylib SetMasterVolume pulse – real SFX needs assets
    if (!ready) return;
    // No-op until footstep wav is added; structure is ready
}

void AudioManager::Shutdown() {
    if (hasFootstep) UnloadSound(footstep);
    if (ready) CloseAudioDevice();
    ready = false;
}
