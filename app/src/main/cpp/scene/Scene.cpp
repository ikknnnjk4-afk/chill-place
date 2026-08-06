#include "Scene.h"

void Scene::Load() {
    room.Load();
}

void Scene::Update(float dt, Vector3 playerPos) {
    room.Update(dt, playerPos);
}

void Scene::Draw() const {
    room.Draw();
}

void Scene::Unload() {
    room.Unload();
}
