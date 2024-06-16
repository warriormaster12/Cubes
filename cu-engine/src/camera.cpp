#include "camera.h"
#include "render_device/render_device.h"
#include <glm.hpp>
#include <ext/matrix_clip_space.hpp>
#include <ext/matrix_transform.hpp>

Camera::Camera() {
    device = CuRenderDevice::get_singleton();
    if (!device) {
        ENGINE_WARN("Can't initialize camera properly. Make sure that render device is created first");
        return;
    }
    // we are going to calculate view and projection on the gpu
    camera_buffer = device->create_buffer(sizeof(glm::mat4) * 2, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    fov = 90;
    znear = 0.01;
    zfar = 100;
}

void Camera::update() {
    if (!device) {
        return;
    }

    VkExtent2D extent = device->get_swapchain_size();
    if (extent.width != width || extent.height != height) {
        width = extent.width;
        height = extent.height;

        struct GPUCameraData {
            glm::mat4 projection;
            glm::mat4 view;
        }camera_data;
        camera_data.projection = glm::perspective(fov, width / height, znear, zfar);
        camera_data.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        device->write_buffer(&camera_data, sizeof(GPUCameraData), camera_buffer);
    }
}