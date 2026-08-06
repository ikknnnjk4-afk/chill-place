#pragma once
#include "raylib.h"

enum class MenuPhase {
    TITLE,          // 5s immersive title
    FADE_OUT,       // fade to black
    CONTROLLER_SCAN,// "Recherche des manettes..."
    DONE            // go to game
};

class Menu {
public:
    void Load();
    // returns true when ready to enter the 3D world
    bool Update();
    void Draw() const;
    void Unload();

private:
    Texture2D texTitle = {};
    bool      loaded   = false;
    MenuPhase phase    = MenuPhase::TITLE;
    float     timer    = 0.0f;
    float     fade     = 0.0f;
    float     pulse    = 0.0f;
    int       scanDots = 0;
};
