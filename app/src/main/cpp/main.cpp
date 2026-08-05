/*
 * Chill Place – Main entry point
 * Raylib handles Android NativeActivity boilerplate.
 * Content unchanged – only crash-hardening.
 */

#include "raylib.h"
#include "GameApp.h"
#include <android/log.h>

#define LOG_TAG "ChillPlace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int main(void) {
    LOGI("=== Chill Place starting ===");

    // ONLY safe flags. MSAA_4X_HINT removed: crashes on some Mali-G57 (Helio G99).
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);

    InitWindow(0, 0, "Chill Place");

    if (!IsWindowReady()) {
        LOGE("InitWindow FAILED – aborting");
        return 1;
    }

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    LOGI("Window ready: %dx%d", sw, sh);

    if (sw <= 0 || sh <= 0) {
        LOGE("Invalid screen size %dx%d – aborting", sw, sh);
        CloseWindow();
        return 1;
    }

    SetTargetFPS(60);

    GameApp game;
    LOGI("GameApp::Init...");
    game.Init();
    LOGI("GameApp::Init done – entering loop");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        if (dt < 0.0f)  dt = 0.0f;

        game.Update(dt);

        BeginDrawing();
        game.Draw();
        EndDrawing();
    }

    LOGI("Shutting down");
    game.Shutdown();
    CloseWindow();
    return 0;
}
