#include "Scene.h"
#include "raylib.h"

void Scene::Load() {
    room.Load();
}

void Scene::Update(float dt) {
    yoyoAngle += 90.0f * dt; // Spin the yoyo
    if (yoyoAngle >= 360.0f) yoyoAngle -= 360.0f;
}

void Scene::Draw() const {
    // Ambient lighting hint via clear colour visible between meshes
    // Raylib basic lighting is handled per-mesh; we use white light
    room.Draw();
}

void Scene::Unload() {
    room.Unload();
}
