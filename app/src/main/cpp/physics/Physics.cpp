#include "Physics.h"
#include <cmath>

bool Physics::CollidesBox(float x, float z, float cx, float cz, float hw, float hd) {
    return (x > cx - hw && x < cx + hw && z > cz - hd && z < cz + hd);
}

void Physics::ConstrainPlayer(Vector3& pos) {
    // Room walls
    const float RW = 4.7f;
    const float RD_MIN = 0.3f;
    const float RD_MAX = 9.7f;

    if (pos.x < -RW)    pos.x = -RW;
    if (pos.x >  RW)    pos.x =  RW;
    if (pos.z < RD_MIN) pos.z = RD_MIN;
    if (pos.z > RD_MAX) pos.z = RD_MAX;

    // Sofa obstacle (back of room)
    if (CollidesBox(pos.x, pos.z, 0.0f, 8.5f, 2.0f, 1.2f)) {
        // Push player out
        if (pos.z < 8.5f) pos.z = 7.3f;
    }
}
