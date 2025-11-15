#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H
#include "../../core/include/Camera.h"
class CameraController {
public:
    Camera* cam{nullptr};
    void setCamera(Camera* c);
    void move(float dx,float dy,float dz);
    void rotate(float yaw,float pitch);
    void zoom(float scale);
    void reset(); // Скидає камеру на початкову позицію
    
    // Налаштування початкової позиції
    void setInitialPosition(float x, float y, float z, float yaw, float pitch, float fov = 60.0f);
    
private:
    // Початкові параметри камери
    float initialX{4.0f}, initialY{3.0f}, initialZ{4.0f};
    float initialYaw{-135.0f}, initialPitch{-20.0f}, initialFov{60.0f};
};
#endif // CAMERACONTROLLER_H
