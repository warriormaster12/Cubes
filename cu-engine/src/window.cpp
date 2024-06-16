#include "window.h"
#include <GLFW/glfw3.h>
#include "logger.h"

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    CuWindow *cu_window = reinterpret_cast<CuWindow*>(glfwGetWindowUserPointer(window));
    cu_window->width = width;
    cu_window->height = height;
    cu_window->resize = true;
}

bool CuWindow::init(const std::string p_title, int p_width, int p_height) {
    if (!glfwInit()) {
        ENGINE_ERROR("Failed to init glfw");
        return -1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    raw_window = glfwCreateWindow(p_width,p_height, p_title.data(), nullptr, nullptr);
    glfwSetWindowUserPointer(raw_window, this);
    glfwSetFramebufferSizeCallback(raw_window, framebuffer_resize_callback);
    width = p_width;
    height = p_height;
    ENGINE_INFO("Window:'{}' ready", p_title);
    return true;
}

void CuWindow::poll_events() {
    glfwPollEvents();
}

void CuWindow::clear() {
    glfwDestroyWindow(raw_window);
    glfwTerminate();
}

bool CuWindow::should_close() const {
    return glfwWindowShouldClose(raw_window);
}