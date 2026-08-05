/*
 * Chill Place – Main entry point
 * Raylib handles Android NativeActivity boilerplate.
 * We simply provide main() and the game loop.
 */

#include "raylib.h"
#include "GameApp.h"

int main(void) {
    // Full-screen on Android (0,0 → device resolution)
    InitWindow(0, 0, "Chill Place");
    SetTargetFPS(60);

    // Prevent screen from sleeping
    SetWindowState(FLAG_WINDOW_ALWAYS_RUN);

    GameApp game;
    game.Init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f; // Cap delta at 50ms

        game.Update(dt);

        BeginDrawing();
        game.Draw();
        EndDrawing();
    }

    game.Shutdown();
    CloseWindow();
    return 0;
}
