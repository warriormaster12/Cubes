#pragma once
#include "renderer.h"
#include "window.h"

class CuEngine {
public:
    CuEngine();
    CuEngine(const std::string& p_title);
    bool running() const;
    void clear();

    CuWindow window;
private:
    bool ready = false;
    CuRenderer renderer;
};
