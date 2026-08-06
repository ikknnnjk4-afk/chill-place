#pragma once
#include "Room.h"
#include "raylib.h"

class Scene {
public:
    void Load();
    void Update(float dt, Vector3 playerPos);
    void Draw() const;
    void Unload();

    Room& GetRoom() { return room; }
    const Room& GetRoom() const { return room; }

private:
    Room room;
};
