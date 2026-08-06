#include "InputManager.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>

void InputManager::Init() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (sw <= 0) sw = 1280;
    if (sh <= 0) sh = 720;
    // Virtual stick bottom-left (fallback when no physical controller)
    joyCenter = { (float)(sw * 0.14f), (float)(sh * 0.78f) };
    SetGesturesEnabled(GESTURE_TAP | GESTURE_DRAG | GESTURE_HOLD);
}

void InputManager::Update() {
    lookDelta = { 0.0f, 0.0f };
    moveDelta = { 0.0f, 0.0f };
    gyroDelta = { 0.0f, 0.0f };

    UpdateTouch();
    UpdateGyroscope();

    // Physical gamepad (Joy-Con / Xbox / PS) if connected
    if (IsGamepadAvailable(0)) {
        float lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        // Deadzone
        if (fabsf(lx) < 0.15f) lx = 0;
        if (fabsf(ly) < 0.15f) ly = 0;
        // ly is typically inverted on gamepads (up = -1) → flip so up = forward
        moveDelta.x = lx;
        moveDelta.y = -ly;

        float rx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        float ry = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
        if (fabsf(rx) < 0.12f) rx = 0;
        if (fabsf(ry) < 0.12f) ry = 0;
        lookDelta.x += rx * 18.0f;
        lookDelta.y += ry * 18.0f;
    }
}

void InputManager::UpdateTouch() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    if (sw <= 0) return;

    const float joyRadius = sw * 0.09f;
    int count = GetTouchPointCount();
    if (count > 10) count = 10;

    // Also accept mouse for emulator testing
    if (count == 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mp = GetMousePosition();
        if (mp.x < sw * 0.35f) {
            Vector2 delta = Vector2Subtract(mp, joyCenter);
            float d = Vector2Length(delta);
            if (d > joyRadius) delta = Vector2Scale(Vector2Normalize(delta), joyRadius);
            // Screen Y grows downward → invert so finger-up = positive forward
            moveDelta = { delta.x / joyRadius, -delta.y / joyRadius };
        } else if (mp.x > sw * 0.4f) {
            static Vector2 prev = mp;
            if (!lookActive) { lookActive = true; lookPrev = mp; }
            lookDelta = Vector2Subtract(mp, lookPrev);
            lookPrev = mp;
        }
        return;
    }

    if (count == 0) {
        joyActive = false; joyFinger = -1;
        lookActive = false; lookFinger = -1;
        return;
    }

    for (int i = 0; i < count; i++) {
        Vector2 tp = GetTouchPosition(i);
        float distToJoy = Vector2Distance(tp, joyCenter);

        if (!joyActive && distToJoy < joyRadius * 2.2f && tp.x < sw * 0.4f) {
            joyActive = true;
            joyFinger = i;
        }
        if (!lookActive && tp.x > sw * 0.35f) {
            lookActive = true;
            lookFinger = i;
            lookPrev = tp;
        }

        if (joyActive && i == joyFinger) {
            Vector2 delta = Vector2Subtract(tp, joyCenter);
            float d = Vector2Length(delta);
            if (d > joyRadius) delta = Vector2Scale(Vector2Normalize(delta), joyRadius);
            // Invert Y: finger toward top of screen = walk forward
            moveDelta = { delta.x / joyRadius, -delta.y / joyRadius };
        }
        if (lookActive && i == lookFinger) {
            lookDelta = Vector2Subtract(tp, lookPrev);
            lookPrev = tp;
        }
    }
}

void InputManager::UpdateGyroscope() {
    // Raylib Android backend does not expose gyro by default yet.
    // Placeholder for future SensorManager bridge.
    gyroDelta = { 0.0f, 0.0f };
}

void InputManager::Shutdown() {}
