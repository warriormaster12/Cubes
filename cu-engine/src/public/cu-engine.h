#pragma once
#include "renderer.h"
#include "window.h"

class CuEngine {
public:
    CuEngine();
    bool running() const;
    void clear();

    CuWindow window;
    CuRenderer renderer;
private:
    bool ready = false;
};
