#pragma once
#include "render_device/utils.h"

class CuRenderDevice;

class Camera {
public:
    float fov;
    float znear;
    float zfar;
    Buffer camera_buffer;
    Camera();
    void update();
private:
    CuRenderDevice* device;
    float width, height;
};