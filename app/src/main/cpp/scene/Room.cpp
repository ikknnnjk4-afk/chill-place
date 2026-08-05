#include "Room.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

// ── Helper: Draw a textured quad ────────────────────────────────────────────
static void DrawTexturedPlane(Texture2D tex,
                              Vector3 pos, Vector3 normal,
                              float w, float h, float tileU, float tileV)
{
    // Build a mesh on the fly via rlgl for full texture control
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

    // Textures — paths are relative to Android assets directory
    texFloor   = LoadTexture("textures/texture_de_sol_.jpg");
    texCeiling = LoadTexture("textures/texture_de_plafond.jpg");
    texWall    = LoadTexture("textures/texture_de_mur.jpg");

    // Enable texture repeat
    SetTextureFilter(texFloor,   TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(texCeiling, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(texWall,    TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(texFloor,   TEXTURE_WRAP_REPEAT);
    SetTextureWrap(texCeiling, TEXTURE_WRAP_REPEAT);
    SetTextureWrap(texWall,    TEXTURE_WRAP_REPEAT);

    // 3D models
    modelSofa = LoadModel("models/canape.glb");
    modelYoyo = LoadModel("models/yoyo.glb");

    loaded = true;
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
    // Back wall  (z = DEPTH, facing -Z)
    DrawTexturedPlane(texWall,
        { 0.0f, HEIGHT/2, DEPTH },
        { 0.0f, 0.0f, -1.0f },
        WIDTH, HEIGHT, 3.0f, 1.0f);

    // Front wall (z = 0, facing +Z)
    DrawTexturedPlane(texWall,
        { 0.0f, HEIGHT/2, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        WIDTH, HEIGHT, 3.0f, 1.0f);

    // Left wall  (x = -W/2, facing +X)
    DrawTexturedPlane(texWall,
        { -WIDTH/2, HEIGHT/2, DEPTH/2 },
        { 1.0f, 0.0f, 0.0f },
        DEPTH, HEIGHT, 3.0f, 1.0f);

    // Right wall (x = +W/2, facing -X)
    DrawTexturedPlane(texWall,
        { WIDTH/2, HEIGHT/2, DEPTH/2 },
        { -1.0f, 0.0f, 0.0f },
        DEPTH, HEIGHT, 3.0f, 1.0f);
}

void Room::DrawFurniture() const {
    // Sofa at the back of the room, centred
    DrawModel(modelSofa, { 0.0f, 0.0f, 8.5f }, 1.0f, WHITE);

    // Yoyo slightly to the right, mid-room
    DrawModel(modelYoyo, { 1.5f, 0.8f, 5.0f }, 0.6f, WHITE);
}

void Room::Unload() {
    if (!loaded) return;
    UnloadTexture(texFloor);
    UnloadTexture(texCeiling);
    UnloadTexture(texWall);
    UnloadModel(modelSofa);
    UnloadModel(modelYoyo);
    loaded = false;
}
