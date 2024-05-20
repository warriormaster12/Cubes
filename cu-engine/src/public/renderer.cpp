#include "renderer.h"
#include "render_device/render_device.h"
#include "window.h"
#include "logger.h"

CuRenderDevice device;


bool CuRenderer::init(CuWindow *p_window) {
    bool result = false;
    result = device.init(p_window);
    if (result) {
        ENGINE_INFO("Renderer ready");
    }
    return result;
}

void CuRenderer::draw() {
    device.draw();
}

void CuRenderer::clear() {
    device.clear();
}