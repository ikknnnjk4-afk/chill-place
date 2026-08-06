#include "Room.h"
#include "CrashReporter.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <android/log.h>
#include <cmath>
#include <algorithm>

#define LOG_TAG "ChillPlace"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── Textured plane with correct UV orientation ──────────────────────────────
// tileU/tileV control how many times the texture repeats.
// flipV = true flips vertical UV (fixes upside-down wall/ceiling textures).
static void DrawTexturedPlane(Texture2D tex, Vector3 center, Vector3 normal,
                              float width, float height,
                              float tileU, float tileV, bool flipV)
{
    if (tex.id == 0) {
        // Solid fallback
        Color c = { 60, 55, 50, 255 };
        if (fabsf(normal.y) > 0.9f) {
            DrawCube(center, width, 0.04f, height, c);
        } else {
            DrawCube(center, (fabsf(normal.x)>0.5f)?0.04f:width,
                     height, (fabsf(normal.z)>0.5f)?0.04f:width, c);
        }
        return;
    }

    Vector3 right;
    if (fabsf(normal.y) > 0.9f) {
        right = { 1.0f, 0.0f, 0.0f };
    } else {
        right = Vector3Normalize(Vector3CrossProduct({ 0.0f, 1.0f, 0.0f }, normal));
    }
    Vector3 up = Vector3Normalize(Vector3CrossProduct(normal, right));

    Vector3 p0 = Vector3Add(center, Vector3Add(Vector3Scale(right, -width*0.5f), Vector3Scale(up, -height*0.5f)));
    Vector3 p1 = Vector3Add(center, Vector3Add(Vector3Scale(right,  width*0.5f), Vector3Scale(up, -height*0.5f)));
    Vector3 p2 = Vector3Add(center, Vector3Add(Vector3Scale(right,  width*0.5f), Vector3Scale(up,  height*0.5f)));
    Vector3 p3 = Vector3Add(center, Vector3Add(Vector3Scale(right, -width*0.5f), Vector3Scale(up,  height*0.5f)));

    float v0 = flipV ? tileV : 0.0f;
    float v1 = flipV ? 0.0f  : tileV;

    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        rlNormal3f(normal.x, normal.y, normal.z);
        rlTexCoord2f(0.0f,  v0); rlVertex3f(p0.x, p0.y, p0.z);
        rlTexCoord2f(tileU, v0); rlVertex3f(p1.x, p1.y, p1.z);
        rlTexCoord2f(tileU, v1); rlVertex3f(p2.x, p2.y, p2.z);
        rlTexCoord2f(0.0f,  v1); rlVertex3f(p3.x, p3.y, p3.z);
    rlEnd();
    rlSetTexture(0);
}

BoundingBox Room::ComputeModelBounds(const Model& m)
{
    BoundingBox bb = { { 1e9f,1e9f,1e9f }, { -1e9f,-1e9f,-1e9f } };
    if (m.meshCount <= 0 || !m.meshes) return bb;
    for (int i = 0; i < m.meshCount; i++) {
        Mesh& mesh = m.meshes[i];
        if (!mesh.vertices || mesh.vertexCount <= 0) continue;
        for (int v = 0; v < mesh.vertexCount; v++) {
            float x = mesh.vertices[v*3+0];
            float y = mesh.vertices[v*3+1];
            float z = mesh.vertices[v*3+2];
            if (x < bb.min.x) bb.min.x = x;
            if (y < bb.min.y) bb.min.y = y;
            if (z < bb.min.z) bb.min.z = z;
            if (x > bb.max.x) bb.max.x = x;
            if (y > bb.max.y) bb.max.y = y;
            if (z > bb.max.z) bb.max.z = z;
        }
    }
    return bb;
}

void Room::LoadTextures()
{
    texFloor   = LoadTexture("textures/texture_de_sol_.jpg");
    texCeiling = LoadTexture("textures/texture_de_plafond.jpg");
    texWall    = LoadTexture("textures/texture_de_mur.jpg");

    LOGI("Textures floor=%u ceil=%u wall=%u", texFloor.id, texCeiling.id, texWall.id);

    auto setup = [](Texture2D& t) {
        if (t.id == 0) return;
        SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
        GenTextureMipmaps(&t);
    };
    setup(texFloor);
    setup(texCeiling);
    setup(texWall);
}

void Room::LoadModels()
{
    modelSofa = LoadModel("models/canape.glb");
    modelYoyo = LoadModel("models/yoyo.glb");

    LOGI("Sofa meshes=%d Yoyo meshes=%d", modelSofa.meshCount, modelYoyo.meshCount);

    // Auto-scale sofa to ~2.2m wide (human living-room size)
    if (modelSofa.meshCount > 0) {
        BoundingBox bb = ComputeModelBounds(modelSofa);
        float sizeX = bb.max.x - bb.min.x;
        float sizeY = bb.max.y - bb.min.y;
        float sizeZ = bb.max.z - bb.min.z;
        float maxDim = fmaxf(sizeX, fmaxf(sizeY, sizeZ));
        LOGI("Sofa bounds: %.2f x %.2f x %.2f", sizeX, sizeY, sizeZ);

        if (maxDim > 0.01f) {
            // Target: sofa ~2.4m wide, sitting height ~0.45m seat
            float targetWidth = 2.4f;
            sofaScale = targetWidth / fmaxf(sizeX, 0.01f);
            // Clamp ridiculous scales
            if (sofaScale < 0.001f) sofaScale = 0.001f;
            if (sofaScale > 50.0f)  sofaScale = 50.0f;

            float scaledHeight = sizeY * sofaScale;
            // Place on floor: bottom of model at y=0
            float bottomY = bb.min.y * sofaScale;
            sofaPos = {
                0.0f,
                -bottomY,          // sit on floor
                DEPTH * 0.5f - 1.2f // near the back wall
            };
            LOGI("Sofa scale=%.4f pos=(%.2f,%.2f,%.2f) height=%.2f",
                 sofaScale, sofaPos.x, sofaPos.y, sofaPos.z, scaledHeight);
        } else {
            sofaScale = 1.0f;
            sofaPos = { 0.0f, 0.0f, DEPTH * 0.5f - 1.5f };
        }
    }

    // Place TV in front of sofa, facing the room entrance
    tvPos = { 0.0f, 0.0f, sofaPos.z - 2.8f };

    // Mocho in the middle of the room
    catPos = { 0.0f, 0.0f, DEPTH * 0.35f };

    // Mug on a low table near center-left
    mugPos = { -1.5f, 0.72f, 4.5f };
}

void Room::Load()
{
    if (loaded) return;
    CrashReporter_Log("Room::Load");
    LoadTextures();
    LoadModels();
    loaded = true;
    CrashReporter_Log("Room::Load done");
}

void Room::Update(float dt, Vector3 playerPos)
{
    // Cat looks at player (head yaw only)
    Vector3 toPlayer = Vector3Subtract(playerPos, catPos);
    float targetYaw = atan2f(toPlayer.x, toPlayer.z);
    float diff = targetYaw - catHeadYaw;
    while (diff >  3.14159f) diff -= 6.28318f;
    while (diff < -3.14159f) diff += 6.28318f;
    catHeadYaw += diff * fminf(1.0f, dt * 3.0f);

    catBreath += dt * 2.2f;
    catBlink  += dt;
    if (catBlink > 3.5f) catBlink = 0.0f;

    yoyoSpin += dt * 120.0f;
    if (yoyoSpin > 360.0f) yoyoSpin -= 360.0f;

    // Simple mug physics when not held
    if (!mugHeld) {
        mugVel.y -= 9.8f * dt;
        mugPos = Vector3Add(mugPos, Vector3Scale(mugVel, dt));
        // Floor collision
        if (mugPos.y < 0.08f) {
            mugPos.y = 0.08f;
            mugVel.y = 0.0f;
            mugVel.x *= 0.7f;
            mugVel.z *= 0.7f;
        }
        // Room walls
        float hw = WIDTH * 0.5f - 0.2f;
        float hd = DEPTH * 0.5f - 0.2f;
        if (mugPos.x < -hw) { mugPos.x = -hw; mugVel.x = 0; }
        if (mugPos.x >  hw) { mugPos.x =  hw; mugVel.x = 0; }
        if (mugPos.z < -hd) { mugPos.z = -hd; mugVel.z = 0; }
        if (mugPos.z >  hd) { mugPos.z =  hd; mugVel.z = 0; }
    }
}

void Room::DrawFloor() const
{
    // Floor: normal up, no flip, tile ~4x so pattern isn't huge/stretched
    DrawTexturedPlane(texFloor,
        { 0.0f, 0.0f, DEPTH * 0.5f },
        { 0.0f, 1.0f, 0.0f },
        WIDTH, DEPTH, 4.0f, 4.0f, false);
}

void Room::DrawCeiling() const
{
    // Ceiling: normal down — flip V so pattern isn't upside-down
    DrawTexturedPlane(texCeiling,
        { 0.0f, HEIGHT, DEPTH * 0.5f },
        { 0.0f, -1.0f, 0.0f },
        WIDTH, DEPTH, 3.0f, 3.0f, true);
}

void Room::DrawWalls() const
{
    // Walls: moderate tiling, flipV so bricks/patterns sit upright
    const float tileU = 3.0f;
    const float tileV = 1.2f;

    // Back wall (far end, behind sofa)
    DrawTexturedPlane(texWall,
        { 0.0f, HEIGHT*0.5f, DEPTH },
        { 0.0f, 0.0f, -1.0f },
        WIDTH, HEIGHT, tileU, tileV, true);

    // Front wall (near entrance)
    DrawTexturedPlane(texWall,
        { 0.0f, HEIGHT*0.5f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        WIDTH, HEIGHT, tileU, tileV, true);

    // Left wall
    DrawTexturedPlane(texWall,
        { -WIDTH*0.5f, HEIGHT*0.5f, DEPTH*0.5f },
        { 1.0f, 0.0f, 0.0f },
        DEPTH, HEIGHT, tileU, tileV, true);

    // Right wall
    DrawTexturedPlane(texWall,
        { WIDTH*0.5f, HEIGHT*0.5f, DEPTH*0.5f },
        { -1.0f, 0.0f, 0.0f },
        DEPTH, HEIGHT, tileU, tileV, true);
}

void Room::DrawSofa() const
{
    if (modelSofa.meshCount > 0 && modelSofa.meshes) {
        DrawModel(modelSofa, sofaPos, sofaScale, WHITE);
    } else {
        // Placeholder sofa
        DrawCube({ sofaPos.x, 0.4f, sofaPos.z }, 2.4f, 0.8f, 1.0f, { 90, 70, 110, 255 });
        DrawCube({ sofaPos.x, 0.7f, sofaPos.z + 0.35f }, 2.4f, 0.5f, 0.3f, { 80, 60, 100, 255 });
    }
}

void Room::DrawTable() const
{
    // Low coffee table in front of sofa area
    Vector3 t = { 0.0f, 0.35f, tvPos.z + 1.2f };
    DrawCube(t, 1.4f, 0.08f, 0.7f, { 70, 55, 40, 255 });
    DrawCube({ t.x - 0.55f, 0.17f, t.z - 0.25f }, 0.08f, 0.34f, 0.08f, { 50, 40, 30, 255 });
    DrawCube({ t.x + 0.55f, 0.17f, t.z - 0.25f }, 0.08f, 0.34f, 0.08f, { 50, 40, 30, 255 });
    DrawCube({ t.x - 0.55f, 0.17f, t.z + 0.25f }, 0.08f, 0.34f, 0.08f, { 50, 40, 30, 255 });
    DrawCube({ t.x + 0.55f, 0.17f, t.z + 0.25f }, 0.08f, 0.34f, 0.08f, { 50, 40, 30, 255 });
}

void Room::DrawTv() const
{
    // Procedural CRT TV (until real GLB from Tripo is added)
    Vector3 base = tvPos;
    // Stand / cabinet
    DrawCube({ base.x, 0.25f, base.z }, 1.1f, 0.5f, 0.55f, { 55, 40, 30, 255 });
    // CRT body
    DrawCube({ base.x, 0.85f, base.z }, 1.0f, 0.7f, 0.5f, { 40, 35, 45, 255 });
    // Bezel
    DrawCube({ base.x, 0.88f, base.z - 0.22f }, 0.85f, 0.55f, 0.06f, { 25, 25, 30, 255 });
    // Screen
    Color screenCol = tvOn ? (Color){ 40, 80, 160, 255 } : (Color){ 10, 10, 15, 255 };
    DrawCube({ base.x, 0.88f, base.z - 0.26f }, 0.75f, 0.48f, 0.02f, screenCol);

    if (tvOn) {
        // Simple UI text floating on screen plane (2D approx in 3D via billboard-ish cubes)
        // Real screen UI would use a RenderTexture mapped onto the quad; this is a visible placeholder
        DrawCube({ base.x - 0.18f, 0.95f, base.z - 0.28f }, 0.28f, 0.08f, 0.01f, { 200, 200, 255, 255 });
        DrawCube({ base.x + 0.18f, 0.95f, base.z - 0.28f }, 0.28f, 0.08f, 0.01f, { 200, 200, 255, 255 });
    }
}

void Room::DrawMug() const
{
    // Procedural ceramic mug (until Tripo model)
    Vector3 p = mugPos;
    // Body
    DrawCylinder(p, 0.06f, 0.07f, 0.12f, 16, { 240, 230, 245, 255 });
    // Handle (torus approx with small cube arc)
    DrawCube({ p.x + 0.09f, p.y + 0.06f, p.z }, 0.03f, 0.06f, 0.02f, { 230, 220, 240, 255 });
    // Outline-ish darker rim
    DrawCylinderWires(p, 0.07f, 0.08f, 0.12f, 12, { 30, 20, 40, 255 });
}

void Room::DrawYoyo() const
{
    Vector3 yp = { 2.0f, 0.9f, 5.5f };
    if (modelYoyo.meshCount > 0 && modelYoyo.meshes) {
        // Scale yoyo to ~15cm
        BoundingBox bb = ComputeModelBounds(modelYoyo);
        float maxDim = fmaxf(bb.max.x - bb.min.x, fmaxf(bb.max.y - bb.min.y, bb.max.z - bb.min.z));
        float sc = (maxDim > 0.01f) ? (0.15f / maxDim) : 0.15f;
        rlPushMatrix();
        rlTranslatef(yp.x, yp.y, yp.z);
        rlRotatef(yoyoSpin, 0, 1, 0);
        rlScalef(sc, sc, sc);
        DrawModel(modelYoyo, { 0, 0, 0 }, 1.0f, WHITE);
        rlPopMatrix();
    } else {
        DrawSphere(yp, 0.08f, { 220, 80, 80, 255 });
    }
}

void Room::DrawMocho() const
{
    // Simple procedural cat – sitting, looks at player via head yaw
    float breath = 1.0f + 0.03f * sinf(catBreath);
    Vector3 body = { catPos.x, 0.22f * breath, catPos.z };

    // Body
    DrawSphere(body, 0.22f, { 255, 160, 60, 255 });
    // Head
    Vector3 head = {
        body.x + sinf(catHeadYaw) * 0.18f,
        body.y + 0.28f,
        body.z + cosf(catHeadYaw) * 0.18f
    };
    DrawSphere(head, 0.14f, { 255, 170, 70, 255 });

    // Ears
    Vector3 earL = { head.x - 0.08f, head.y + 0.14f, head.z };
    Vector3 earR = { head.x + 0.08f, head.y + 0.14f, head.z };
    DrawCylinder(earL, 0.0f, 0.05f, 0.1f, 6, { 255, 150, 50, 255 });
    DrawCylinder(earR, 0.0f, 0.05f, 0.1f, 6, { 255, 150, 50, 255 });

    // Eyes (blink)
    bool closed = (catBlink > 3.2f);
    if (!closed) {
        DrawSphere({ head.x - 0.05f, head.y + 0.02f, head.z + 0.1f }, 0.025f, BLACK);
        DrawSphere({ head.x + 0.05f, head.y + 0.02f, head.z + 0.1f }, 0.025f, BLACK);
    } else {
        DrawCube({ head.x - 0.05f, head.y + 0.02f, head.z + 0.1f }, 0.04f, 0.008f, 0.02f, BLACK);
        DrawCube({ head.x + 0.05f, head.y + 0.02f, head.z + 0.1f }, 0.04f, 0.008f, 0.02f, BLACK);
    }

    // Tail
    DrawSphere({ body.x - 0.2f, body.y + 0.05f, body.z - 0.15f }, 0.06f, { 255, 150, 50, 255 });
}

void Room::Draw() const
{
    DrawFloor();
    DrawCeiling();
    DrawWalls();
    DrawSofa();
    DrawTable();
    DrawTv();
    DrawMug();
    DrawYoyo();
    DrawMocho();
}

void Room::Unload()
{
    if (!loaded) return;
    if (texFloor.id)   UnloadTexture(texFloor);
    if (texCeiling.id) UnloadTexture(texCeiling);
    if (texWall.id)    UnloadTexture(texWall);
    if (modelSofa.meshCount > 0) UnloadModel(modelSofa);
    if (modelYoyo.meshCount > 0) UnloadModel(modelYoyo);
    texFloor = texCeiling = texWall = {};
    modelSofa = modelYoyo = {};
    loaded = false;
}
