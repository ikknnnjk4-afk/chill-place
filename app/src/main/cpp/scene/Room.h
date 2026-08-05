#pragma once
#include "raylib.h"

class Room {
public:
    void Load();
    void Draw() const;
    void Unload();

private:
    // Room dimensions
    static constexpr float WIDTH  = 10.0f;
    static constexpr float HEIGHT =  3.2f;
    static constexpr float DEPTH  = 10.0f;

    // Surface textures
    Texture2D texFloor   = {};
    Texture2D texCeiling = {};
    Texture2D texWall    = {};

    // 3D models
    Model modelSofa  = {};
    Model modelYoyo  = {};

    bool loaded = false;

    void DrawFloor()   const;
    void DrawCeiling() const;
    void DrawWalls()   const;
    void DrawFurniture() const;
};
