#include "CameraController.h"

void CameraController::setCamera(Camera* c) { cam = c; }
void CameraController::move(float dx, float dy, float dz) { if(cam) cam->move(dx, dy, dz); }
void CameraController::rotate(float yaw, float pitch) { if(cam) cam->rotate(yaw, pitch); }
void CameraController::zoom(float scale) {
    if(!cam) return;
    cam->fov *= scale;
    if(cam->fov < 10.0f) cam->fov = 10.0f;
    if(cam->fov > 120.0f) cam->fov = 120.0f;
}

void CameraController::reset() {
    if(!cam) return;
    cam->position = {initialX, initialY, initialZ};
    cam->yaw = initialYaw;
    cam->pitch = initialPitch;
    cam->fov = initialFov;
}

void CameraController::setInitialPosition(float x, float y, float z, float yaw, float pitch, float fov) {
    initialX = x;
    initialY = y;
    initialZ = z;
    initialYaw = yaw;
    initialPitch = pitch;
    initialFov = fov;
}