/*
 * Chill Place – Main entry point + crash report screen
 */

#include "raylib.h"
#include "GameApp.h"
#include "CrashReporter.h"
#include <android/log.h>
#include <string>
#include <vector>
#include <sstream>

#define LOG_TAG "ChillPlace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static void ShowCrashReport(const std::string& report) {
    // Simple full-screen text so the user can screenshot and send it.
    SetTargetFPS(30);
    float timer = 0.0f;
    bool dismissed = false;

    // Split into lines for drawing
    std::vector<std::string> lines;
    std::istringstream iss(report);
    std::string line;
    while (std::getline(iss, line)) {
        // wrap long lines roughly
        while (line.size() > 56) {
            lines.push_back(line.substr(0, 56));
            line = line.substr(56);
        }
        lines.push_back(line);
    }

    while (!WindowShouldClose() && !dismissed) {
        float dt = GetFrameTime();
        timer += dt;

        // Tap or wait 90s to continue
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            GetTouchPointCount() > 0 ||
            timer > 90.0f) {
            dismissed = true;
        }

        BeginDrawing();
        ClearBackground({ 20, 10, 10, 255 });

        DrawText("CRASH DETECTE – envoie une capture", 20, 20, 22, RED);
        DrawText("a Grok (ecran suivant = jeu)", 20, 48, 18, LIGHTGRAY);

        int y = 90;
        int maxLines = (GetScreenHeight() - 140) / 18;
        int start = 0;
        if ((int)lines.size() > maxLines) {
            // show the end (most useful: signal + last breadcrumbs)
            start = (int)lines.size() - maxLines;
        }
        for (int i = start; i < (int)lines.size(); i++) {
            DrawText(lines[i].c_str(), 12, y, 16, RAYWHITE);
            y += 18;
        }

        DrawText("Touche l'ecran pour continuer...", 20, GetScreenHeight() - 40, 18, YELLOW);
        EndDrawing();
    }

    CrashReporter_Clear();
}

int main(void) {
    CrashReporter_Install();
    CrashReporter_Log("=== Chill Place starting ===");

    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    CrashReporter_Log("SetConfigFlags done");

    InitWindow(0, 0, "Chill Place");
    CrashReporter_Log("InitWindow returned");

    if (!IsWindowReady()) {
        CrashReporter_Log("InitWindow FAILED");
        LOGE("InitWindow FAILED – aborting");
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

    // Show previous crash if any (THIS is how you send me the log without adb)
    std::string last = CrashReporter_LoadLastCrash();
    if (!last.empty()) {
        CrashReporter_Log("Previous crash found – showing report");
        ShowCrashReport(last);
    }

    SetTargetFPS(60);

    GameApp game;
    CrashReporter_Log("GameApp::Init...");
    game.Init();
    CrashReporter_Log("GameApp::Init done – entering loop");

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
