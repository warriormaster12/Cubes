#pragma once

#include "render_device/utils.h"
#include <glm.hpp>
#include <vector>

class CuRenderDevice;

class Camera {
public:
  float fov;
  float znear;
  float zfar;
  Camera();
  void update();
  bool active = false;
  bool dirty = false;

  struct GPUCameraData {
    glm::mat4 projection;
    glm::mat4 view;
  } gpu_data;

private:
  CuRenderDevice *device;
  float width, height;
};

class CameraManager {
public:
  CameraManager();
  void init();
  Camera *create_camera();
  // Camera *get_camera(int p_idx);
  Buffer &get_camera_buffer() { return camera_buffer; }
  void update_active_camera();
  // void remove_camera(Camera *p_camera);
  void clear();

  static CameraManager *get_singleton();

private:
  static CameraManager *singleton;
  Buffer camera_buffer;

  CuRenderDevice *device;

  std::vector<Camera> cameras = {};
};