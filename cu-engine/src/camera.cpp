#include "camera.h"
#include "render_device/render_device.h"
#include "render_device/utils.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <ext/matrix_clip_space.hpp>
#include <ext/matrix_transform.hpp>
#include <glm.hpp>
#include <gtx/transform.hpp>

Camera::Camera() {
  device = CuRenderDevice::get_singleton();
  if (!device) {
    ENGINE_WARN("Can't initialize camera properly. Make sure that render "
                "device is created first");
    return;
  }

  fov = 90;
  znear = 0.1;
  zfar = 100;
  active = true;
}

void Camera::update() {
  if (!device) {
    return;
  }

  VkExtent2D extent = device->get_swapchain_size();
  if (extent.width != width || extent.height != height) {
    width = extent.width;
    height = extent.height;

    gpu_data.projection = glm::perspective(fov, width / height, znear, zfar);
    gpu_data.view =
        glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    gpu_data.projection[1][1] *= -1;
    dirty = true;
  }
}

CameraManager *CameraManager::singleton = nullptr;

CameraManager::CameraManager() { singleton = this; }
CameraManager *CameraManager::get_singleton() { return singleton; }

void CameraManager::init() {
  device = CuRenderDevice::get_singleton();
  if (device) {
    camera_buffer = device->create_buffer(sizeof(glm::mat4) * 2,
                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                          VMA_MEMORY_USAGE_CPU_TO_GPU);
  }
}

Camera *CameraManager::create_camera() {
  if (!device) {
    ENGINE_ERROR("Device not found");
    return nullptr;
  }
  cameras.push_back(Camera());
  return &cameras[cameras.size() - 1];
}

void CameraManager::update_active_camera() {
  if (!device) {
    return;
  }
  for (Camera &camera : cameras) {
    if (camera.active && camera.dirty) {
      device->write_buffer(&camera.gpu_data, sizeof(camera.gpu_data),
                           camera_buffer);
      break;
    }
  }
}

void CameraManager::clear() {
  cameras.clear();
  if (device) {
    device->clear_buffer(camera_buffer);
  }
}