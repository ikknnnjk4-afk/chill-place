/*
 * Chill Place – native entry (GameActivity)
 * Crash report UI is handled by pure-Java MainActivity.
 */

#include "raylib.h"
#include "GameApp.h"
#include "CrashReporter.h"
#include <android/log.h>
#include <stdio.h>

#define LOG_TAG "ChillPlace"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int main(void) {
    CrashReporter_Install();
    CrashReporter_Log("=== Chill Place native starting ===");

    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    CrashReporter_Log("SetConfigFlags done");

    InitWindow(0, 0, "Chill Place");
    CrashReporter_Log("InitWindow returned");

    if (!IsWindowReady()) {
        CrashReporter_Log("InitWindow FAILED");
        LOGE("InitWindow FAILED");
        return 1;
    }

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    char buf[64];
    snprintf(buf, sizeof(buf), "Window ready: %dx%d", sw, sh);
    CrashReporter_Log(buf);

    if (sw <= 0 || sh <= 0) {
        CrashReporter_Log("Invalid screen size");
        CloseWindow();
        return 1;
    }

    SetTargetFPS(60);

    GameApp game;
    CrashReporter_Log("GameApp::Init...");
    game.Init();
    CrashReporter_Log("GameApp::Init done – loop");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        if (dt < 0.0f)  dt = 0.0f;
        game.Update(dt);
        BeginDrawing();
        game.Draw();
        EndDrawing();
    }

    CrashReporter_Log("Shutting down");
    game.Shutdown();
    CloseWindow();
    return 0;
}
