#include "cu-engine.h"

CuEngine::CuEngine() {
    ready = window.init("test", 1280, 720);
    ready = renderer.init(&window);
}

bool CuEngine::running() const {
    return ready && !window.should_close();
}

void CuEngine::clear() {
    renderer.clear();
    window.clear();
}