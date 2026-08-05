#pragma once
#include "raylib.h"

class AudioManager {
public:
    void Init();
    void PlayAmbient();
    void Shutdown();

private:
    Music ambient = {};
    bool  ready   = false;
};
