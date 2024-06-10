#include "cu-engine.h"

CuEngine::CuEngine(const std::string& p_title) {
    ready = window.init(p_title, 1280, 720);
    ready = renderer.init(&window);
}

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