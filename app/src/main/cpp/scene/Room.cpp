#include "Room.h"
#include "CrashReporter.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <android/log.h>

#define LOG_TAG "ChillPlace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── Helper: Draw a textured quad (safe if texture id == 0) ──────────────────
static void DrawTexturedPlane(Texture2D tex,
                              Vector3 pos, Vector3 normal,
                              float w, float h, float tileU, float tileV)
{
    // Guard: never call rlSetTexture with invalid id (can SIGSEGV on Mali)
    if (tex.id == 0) {
        // Fallback solid plane so the room still exists
        DrawCube(pos, w, 0.02f, h, DARKGRAY);
        return;
    }

    Vector3 right = {1, 0, 0};
    if (fabsf(normal.y) > 0.9f) {
        right = {1, 0, 0};
    } else {
        right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, normal));
    }
    Vector3 up = Vector3Normalize(Vector3CrossProduct(normal, right));

    Vector3 p0 = Vector3Add(pos, Vector3Add(Vector3Scale(right, -w/2), Vector3Scale(up, -h/2)));
    Vector3 p1 = Vector3Add(pos, Vector3Add(Vector3Scale(right,  w/2), Vector3Scale(up, -h/2)));
    Vector3 p2 = Vector3Add(pos, Vector3Add(Vector3Scale(right,  w/2), Vector3Scale(up,  h/2)));
    Vector3 p3 = Vector3Add(pos, Vector3Add(Vector3Scale(right, -w/2), Vector3Scale(up,  h/2)));

    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        rlNormal3f(normal.x, normal.y, normal.z);

        rlTexCoord2f(0.0f,   0.0f);   rlVertex3f(p0.x, p0.y, p0.z);
        rlTexCoord2f(tileU,  0.0f);   rlVertex3f(p1.x, p1.y, p1.z);
        rlTexCoord2f(tileU,  tileV);  rlVertex3f(p2.x, p2.y, p2.z);
        rlTexCoord2f(0.0f,   tileV);  rlVertex3f(p3.x, p3.y, p3.z);
    rlEnd();
    rlSetTexture(0);
}

void Room::Load() {
    if (loaded) return;
    LOGI("Room::Load start"); CrashReporter_Log("Room::Load start");

    texFloor   = LoadTexture("textures/texture_de_sol_.jpg");
    texCeiling = LoadTexture("textures/texture_de_plafond.jpg");
    texWall    = LoadTexture("textures/texture_de_mur.jpg");

    LOGI("Textures: floor=%u ceiling=%u wall=%u", texFloor.id, texCeiling.id, texWall.id);

    if (texFloor.id > 0) {
        SetTextureFilter(texFloor, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(texFloor, TEXTURE_WRAP_REPEAT);
    }
    if (texCeiling.id > 0) {
        SetTextureFilter(texCeiling, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(texCeiling, TEXTURE_WRAP_REPEAT);
    }
    if (texWall.id > 0) {
        SetTextureFilter(texWall, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(texWall, TEXTURE_WRAP_REPEAT);
    }

    LOGI("Loading models..."); CrashReporter_Log("Loading models...");
    modelSofa = LoadModel("models/canape.glb");
    modelYoyo = LoadModel("models/yoyo.glb");
    LOGI("Models: sofa meshes=%d yoyo meshes=%d", modelSofa.meshCount, modelYoyo.meshCount);

    loaded = true;
    LOGI("Room::Load done"); CrashReporter_Log("Room::Load done");
}

void Room::Draw() const {
    if (!loaded) return;
    DrawFloor();
    DrawCeiling();
    DrawWalls();
    DrawFurniture();
}

void Room::DrawFloor() const {
    Vector3 pos    = { 0.0f, 0.0f, DEPTH / 2 };
    Vector3 normal = { 0.0f, 1.0f, 0.0f };
    DrawTexturedPlane(texFloor, pos, normal, WIDTH, DEPTH, 4.0f, 4.0f);
}

void Room::DrawCeiling() const {
    Vector3 pos    = { 0.0f, HEIGHT, DEPTH / 2 };
    Vector3 normal = { 0.0f, -1.0f, 0.0f };
    DrawTexturedPlane(texCeiling, pos, normal, WIDTH, DEPTH, 3.0f, 3.0f);
}

void Room::DrawWalls() const {
    DrawTexturedPlane(texWall,
        { 0.0f, HEIGHT/2, DEPTH },
        { 0.0f, 0.0f, -1.0f },
        WIDTH, HEIGHT, 3.0f, 1.0f);

    DrawTexturedPlane(texWall,
        { 0.0f, HEIGHT/2, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        WIDTH, HEIGHT, 3.0f, 1.0f);

    DrawTexturedPlane(texWall,
        { -WIDTH/2, HEIGHT/2, DEPTH/2 },
        { 1.0f, 0.0f, 0.0f },
        DEPTH, HEIGHT, 3.0f, 1.0f);

    DrawTexturedPlane(texWall,
        { WIDTH/2, HEIGHT/2, DEPTH/2 },
        { -1.0f, 0.0f, 0.0f },
        DEPTH, HEIGHT, 3.0f, 1.0f);
}

void Room::DrawFurniture() const {
    // Only draw if model actually loaded (meshCount > 0)
    if (modelSofa.meshCount > 0 && modelSofa.meshes != nullptr)
        DrawModel(modelSofa, { 0.0f, 0.0f, 8.5f }, 1.0f, WHITE);

    if (modelYoyo.meshCount > 0 && modelYoyo.meshes != nullptr)
        DrawModel(modelYoyo, { 1.5f, 0.8f, 5.0f }, 0.6f, WHITE);
}

void Room::Unload() {
    if (!loaded) return;
    if (texFloor.id > 0)   UnloadTexture(texFloor);
    if (texCeiling.id > 0) UnloadTexture(texCeiling);
    if (texWall.id > 0)    UnloadTexture(texWall);
    if (modelSofa.meshCount > 0) UnloadModel(modelSofa);
    if (modelYoyo.meshCount > 0) UnloadModel(modelYoyo);
    // Reset to zeroed state
    texFloor = texCeiling = texWall = Texture2D{};
    modelSofa = modelYoyo = Model{};
    loaded = false;
}
