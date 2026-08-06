#pragma once
#include "raylib.h"

class Room {
public:
    void Load();
    void Update(float dt, Vector3 playerPos);
    void Draw() const;
    void Unload();

    // Interactive props
    Vector3 GetMugPos() const { return mugPos; }
    void SetMugPos(Vector3 p) { mugPos = p; }
    bool IsMugHeld() const { return mugHeld; }
    void SetMugHeld(bool h) { mugHeld = h; }
    Vector3 GetTvPos() const { return tvPos; }
    bool IsTvOn() const { return tvOn; }
    void SetTvOn(bool on) { tvOn = on; }

    // Room bounds for collision
    static constexpr float WIDTH  = 10.0f;
    static constexpr float HEIGHT =  3.0f;
    static constexpr float DEPTH  = 12.0f; // deeper so sofa is clearly "at the back"

private:
    Texture2D texFloor   = {};
    Texture2D texCeiling = {};
    Texture2D texWall    = {};

    Model modelSofa = {};
    Model modelYoyo = {};
    float sofaScale = 1.0f;
    Vector3 sofaPos = { 0.0f, 0.0f, 0.0f };

    // Procedural / placeholder props
    Vector3 mugPos  = { 1.2f, 0.85f, 4.0f };
    Vector3 mugVel  = { 0.0f, 0.0f, 0.0f };
    bool    mugHeld = false;
    float   mugYaw  = 0.0f;

    Vector3 tvPos   = { 0.0f, 0.55f, 10.2f };
    bool    tvOn    = false;

    // Mocho the cat
    Vector3 catPos  = { 0.0f, 0.0f, 6.0f };
    float   catHeadYaw = 0.0f;
    float   catBlink   = 0.0f;
    float   catBreath  = 0.0f;
    float   yoyoSpin   = 0.0f;

    bool loaded = false;

    void LoadTextures();
    void LoadModels();
    void DrawFloor() const;
    void DrawCeiling() const;
    void DrawWalls() const;
    void DrawSofa() const;
    void DrawTv() const;
    void DrawMug() const;
    void DrawYoyo() const;
    void DrawMocho() const;
    void DrawTable() const;

    static BoundingBox ComputeModelBounds(const Model& m);
};
