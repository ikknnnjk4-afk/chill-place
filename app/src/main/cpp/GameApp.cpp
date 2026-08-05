#include "GameApp.h"
#include "CrashReporter.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>

void GameApp::Init() {
    // Camera setup – first person starting at the front of the room
    camera.position   = { 0.0f, 1.65f, 1.0f };
    camera.target     = { 0.0f, 1.65f, 5.0f };
    camera.up         = { 0.0f, 1.0f, 0.0f };
    camera.fovy       = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    CrashReporter_Log("menu.Load");
    menu.Load();
    CrashReporter_Log("input.Init");
    input.Init();
    CrashReporter_Log("scene.Load");
    scene.Load();
    CrashReporter_Log("hud.Load");
    hud.Load();
    CrashReporter_Log("audio.Init");
    audio.Init();
    CrashReporter_Log("audio.PlayAmbient");
    audio.PlayAmbient();
    CrashReporter_Log("GameApp::Init complete");
}

void GameApp::Update(float dt) {
    input.Update();

    switch (state) {
        case GameState::MENU:
            if (menu.Update()) {
                state = GameState::PLAYING;
            }
            break;

        case GameState::PLAYING:
            UpdateCamera(dt);
            scene.Update(dt);
            hud.Update(dt);

            // Open AI chat when tapping the bottom-right area or back button
            if (hud.ChatButtonPressed()) {
                state = GameState::AI_CHAT;
                hud.OpenChat();
            }
            break;

        case GameState::AI_CHAT:
            hud.UpdateChat(dt);
            if (hud.ChatClosed()) {
                state = GameState::PLAYING;
            }
            break;

        case GameState::PAUSED:
            break;
    }
}

void GameApp::UpdateCamera(float dt) {
    // Touch-based look (drag to rotate)
    Vector2 touchDelta = input.GetLookDelta();

    camYaw   -= touchDelta.x * 0.003f;
    camPitch -= touchDelta.y * 0.003f;

    // Clamp pitch
    if (camPitch >  1.4f) camPitch =  1.4f;
    if (camPitch < -1.4f) camPitch = -1.4f;

    // Gyroscope supplement
    Vector2 gyro = input.GetGyroDelta();
    camYaw   += gyro.x * dt;
    camPitch += gyro.y * dt;

    // Forward vector from yaw/pitch
    Vector3 forward = {
        cosf(camPitch) * sinf(camYaw),
        sinf(camPitch),
        cosf(camPitch) * cosf(camYaw)
    };

    // Walk with virtual joystick (left side of screen)
    Vector2 move = input.GetMoveDelta();
    Vector3 right = Vector3CrossProduct(forward, camera.up);
    right = Vector3Normalize(right);

    float speed = 3.0f * dt;
    camera.position.x += (forward.x * move.y + right.x * move.x) * speed;
    camera.position.z += (forward.z * move.y + right.z * move.x) * speed;

    // Room boundaries
    const float ROOM_HALF = 4.8f;
    const float ROOM_DEPTH = 9.6f;
    if (camera.position.x < -ROOM_HALF) camera.position.x = -ROOM_HALF;
    if (camera.position.x >  ROOM_HALF) camera.position.x =  ROOM_HALF;
    if (camera.position.z < 0.2f)       camera.position.z = 0.2f;
    if (camera.position.z > ROOM_DEPTH) camera.position.z = ROOM_DEPTH;

    camera.target = Vector3Add(camera.position, forward);
}

void GameApp::Draw() {
    ClearBackground(BLACK);

    switch (state) {
        case GameState::MENU:
            menu.Draw();
            break;

        case GameState::PLAYING:
            DrawScene3D();
            hud.Draw();
            break;

        case GameState::AI_CHAT:
            DrawScene3D();
            hud.DrawChat();
            break;

        case GameState::PAUSED:
            DrawScene3D();
            break;
    }
}

void GameApp::DrawScene3D() {
    BeginMode3D(camera);
    scene.Draw();
    EndMode3D();
}

void GameApp::Shutdown() {
    audio.Shutdown();
    scene.Unload();
    menu.Unload();
    hud.Unload();
    input.Shutdown();
}
