#ifndef CAMERA_H
#define CAMERA_H
#include "Vec3.h"
class Camera {
public:
    Vec3 position{0,0,5};
    float yaw{0.f};
    float pitch{0.f};
    float fov{60.f};
    void move(float dx,float dy,float dz){ position.x += dx; position.y += dy; position.z += dz; }
    void rotate(float y,float p){ yaw += y; pitch += p; }
    void zoom(float scale){ fov = fov * scale; if(fov < 10.f) fov = 10.f; if(fov>120.f) fov=120.f; }
};
#endif // CAMERA_H
