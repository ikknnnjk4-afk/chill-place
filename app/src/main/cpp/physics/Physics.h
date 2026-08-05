#pragma once
#include "raylib.h"

// Simple AABB collision helper for the room
class Physics {
public:
    // Returns true if the point (x, z) is inside a box obstacle (centred at cx,cz, half-extents hw,hd)
    static bool CollidesBox(float x, float z, float cx, float cz, float hw, float hd);

    // Clamp a camera position to stay within the room and away from the sofa
    static void ConstrainPlayer(Vector3& pos);
};
