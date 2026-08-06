#pragma once
#include "raylib.h"

class AudioManager {
public:
    void Init();
    void PlayAmbient();
    void PlayFootstep();
    void Shutdown();

private:
    bool ready = false;
    Sound footstep = {};
    bool hasFootstep = false;
};
