#pragma once
#include "raylib.h"

class InputManager {
public:
    void Init();
    void Update();
    void Shutdown();

    // Returns look rotation delta (pitch, yaw) from touch drag
    Vector2 GetLookDelta() const  { return lookDelta; }

    // Returns movement delta (x = strafe, y = forward) from virtual joystick
    Vector2 GetMoveDelta() const  { return moveDelta; }

    // Returns gyroscope delta (yaw, pitch) scaled per frame
    Vector2 GetGyroDelta() const  { return gyroDelta; }

private:
    Vector2 lookDelta = {};
    Vector2 moveDelta = {};
    Vector2 gyroDelta = {};

    // Previous look touch position
    Vector2 lookPrev   = {};
    bool    lookActive = false;
    int     lookFinger = -1;

    // Joystick
    Vector2 joyCenter  = {};
    bool    joyActive  = false;
    int     joyFinger  = -1;

    void UpdateGyroscope();
    void UpdateTouch();
};
