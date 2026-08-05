#pragma once
#include "Room.h"
#include "raylib.h"

class Scene {
public:
    void Load();
    void Update(float dt);
    void Draw() const;
    void Unload();

private:
    Room  room;
    float yoyoAngle = 0.0f;
    float ambientLight[4] = { 0.6f, 0.55f, 0.5f, 1.0f };
};
