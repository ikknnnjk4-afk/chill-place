#pragma once
#include "raylib.h"
#include "scene/Scene.h"
#include "ui/Menu.h"
#include "ui/HUD.h"
#include "audio/AudioManager.h"
#include "input/InputManager.h"

enum class GameState {
    MENU,
    PLAYING,
    AI_CHAT,
    PAUSED
};

class GameApp {
public:
    void Init();
    void Update(float dt);
    void Draw();
    void Shutdown();

private:
    GameState    state = GameState::MENU;
    Scene        scene;
    Menu         menu;
    HUD          hud;
    AudioManager audio;
    InputManager input;

    Camera3D camera = {};
    float    camYaw   = 0.0f;
    float    camPitch = 0.0f;

    // Footstep timer
    float stepTimer = 0.0f;
    bool  wasMoving = false;

    void UpdateCamera(float dt);
    void DrawScene3D();
};
