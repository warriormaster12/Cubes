#pragma once

#include <string>

class GLFWwindow;

class CuWindow {
public: 
    bool init(const std::string p_title, int p_width, int p_height);
    void poll_events();
    void clear();

    bool should_close() const;

    GLFWwindow *raw_window = nullptr;
    int width;
    int height;
    bool resize = false;
};