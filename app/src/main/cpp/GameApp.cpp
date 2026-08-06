#include "GameApp.h"
#include "CrashReporter.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>

void GameApp::Init() {
    // Start near the front of the room, looking toward the back (sofa / TV)
    camera.position   = { 0.0f, 1.65f, 1.5f };
    camera.target     = { 0.0f, 1.65f, 6.0f };
    camera.up         = { 0.0f, 1.0f, 0.0f };
    camera.fovy       = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camYaw   = 0.0f;
    camPitch = 0.0f;

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

        case GameState::PLAYING: {
            UpdateCamera(dt);
            scene.Update(dt, camera.position);
            hud.Update(dt);

            // Footsteps when moving
            Vector2 move = input.GetMoveDelta();
            bool moving = (fabsf(move.x) + fabsf(move.y)) > 0.15f;
            if (moving) {
                stepTimer += dt;
                if (stepTimer > 0.45f) {
                    stepTimer = 0.0f;
                    audio.PlayFootstep();
                }
            } else {
                stepTimer = 0.0f;
            }
            wasMoving = moving;

            if (hud.ChatButtonPressed()) {
                state = GameState::AI_CHAT;
                hud.OpenChat();
            }
            break;
        }

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
    Vector2 touchDelta = input.GetLookDelta();
    // Look: horizontal drag → yaw, vertical → pitch (not inverted)
    camYaw   -= touchDelta.x * 0.0035f;
    camPitch -= touchDelta.y * 0.0035f;

    if (camPitch >  1.35f) camPitch =  1.35f;
    if (camPitch < -1.35f) camPitch = -1.35f;

    Vector2 gyro = input.GetGyroDelta();
    camYaw   += gyro.x * dt;
    camPitch += gyro.y * dt;

    Vector3 forward = {
        cosf(camPitch) * sinf(camYaw),
        sinf(camPitch),
        cosf(camPitch) * cosf(camYaw)
    };
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, { 0, 1, 0 }));

    // Movement: joystick Y positive = forward (NOT inverted)
    Vector2 move = input.GetMoveDelta();
    // move.y > 0 when stick pushed up → walk forward
    float speed = 2.8f * dt;
    camera.position.x += (forward.x * move.y + right.x * move.x) * speed;
    camera.position.z += (forward.z * move.y + right.z * move.x) * speed;

    // Keep player inside room
    const float margin = 0.4f;
    float hw = Room::WIDTH * 0.5f - margin;
    float hd = Room::DEPTH - margin;
    if (camera.position.x < -hw) camera.position.x = -hw;
    if (camera.position.x >  hw) camera.position.x =  hw;
    if (camera.position.z < margin) camera.position.z = margin;
    if (camera.position.z > hd) camera.position.z = hd;

    camera.position.y = 1.65f;
    camera.target = Vector3Add(camera.position, forward);
}

void GameApp::DrawScene3D() {
    BeginMode3D(camera);
        // Soft ambient fill
        DrawCube({ 0, Room::HEIGHT * 0.5f, Room::DEPTH * 0.5f },
                 Room::WIDTH * 0.98f, Room::HEIGHT * 0.98f, Room::DEPTH * 0.98f,
                 { 255, 255, 255, 0 }); // invisible, just for depth
        scene.Draw();
    EndMode3D();
}

void GameApp::Draw() {
    BeginDrawing();
    ClearBackground({ 15, 12, 20, 255 });

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

    EndDrawing();
}

void GameApp::Shutdown() {
    audio.Shutdown();
    hud.Unload();
    scene.Unload();
    menu.Unload();
    input.Shutdown();
}
