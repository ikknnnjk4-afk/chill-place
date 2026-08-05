#pragma once
#include "raylib.h"

class Menu {
public:
    void Load();
    bool Update(); // returns true when user taps "JOUER"
    void Draw() const;
    void Unload();

private:
    Texture2D texTitle  = {};
    bool      loaded    = false;
    float     fadeAlpha = 0.0f;
    float     pulse     = 0.0f;
};
