#include "InputManager.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>

void InputManager::Init() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (sw <= 0) sw = 1280;
    if (sh <= 0) sh = 720;
    joyCenter = { (float)(sw * 0.13f), (float)(sh * 0.75f) };

    SetGesturesEnabled(GESTURE_TAP | GESTURE_DRAG | GESTURE_HOLD);
}

void InputManager::Update() {
    lookDelta = { 0.0f, 0.0f };
    moveDelta = { 0.0f, 0.0f };
    gyroDelta = { 0.0f, 0.0f };

    UpdateTouch();
    UpdateGyroscope();
}

void InputManager::UpdateTouch() {
    int sw = GetScreenWidth();
    if (sw <= 0) return;

    const float joyRadius = sw * 0.09f;
    int count = GetTouchPointCount();

    if (count == 0) {
        joyActive  = false;
        joyFinger  = -1;
        lookActive = false;
        lookFinger = -1;
        return;
    }

    // Clamp count to a sane max (defensive)
    if (count > 10) count = 10;

    for (int i = 0; i < count; i++) {
        Vector2 tp = GetTouchPosition(i);
        float distToJoy = Vector2Distance(tp, joyCenter);

        if (!joyActive && distToJoy < joyRadius * 2.0f) {
            joyActive = true;
            joyFinger = i;
        }

        if (!lookActive && tp.x > sw * 0.3f) {
            lookActive = true;
            lookFinger = i;
            lookPrev   = tp;
        }

        if (joyActive && i == joyFinger) {
            Vector2 delta = Vector2Subtract(tp, joyCenter);
            float d = Vector2Length(delta);
            if (d > joyRadius) delta = Vector2Scale(Vector2Normalize(delta), joyRadius);
            moveDelta = Vector2Scale(delta, 1.0f / joyRadius);
        }

        if (lookActive && i == lookFinger) {
            lookDelta = Vector2Subtract(tp, lookPrev);
            lookPrev  = tp;
        }
    }
}

void InputManager::UpdateGyroscope() {
    // Raylib does not expose raw gyroscope on Android by default.
    gyroDelta = { 0.0f, 0.0f };
}

void InputManager::Shutdown() {}
