/*
 * Chill Place – Main entry point
 * Raylib handles Android NativeActivity boilerplate.
 */

#include "raylib.h"
#include "GameApp.h"
#include <android/log.h>

#define LOG_TAG "ChillPlace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int main(void) {
    LOGI("=== Chill Place starting ===");

    // Critical: flags must be set BEFORE InitWindow on Android
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN | FLAG_MSAA_4X_HINT);

    InitWindow(0, 0, "Chill Place");

    if (!IsWindowReady()) {
        LOGE("InitWindow FAILED – aborting");
        return 1;
    }

    LOGI("Window ready: %dx%d", GetScreenWidth(), GetScreenHeight());
    SetTargetFPS(60);

    GameApp game;
    LOGI("GameApp::Init...");
    game.Init();
    LOGI("GameApp::Init done");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

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
