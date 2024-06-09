#include "renderer.h"
#include "render_device/render_device.h"
#include "window.h"
#include "logger.h"

CuRenderDevice device;

CuRenderer *CuRenderer::singleton = nullptr;

CuRenderer::CuRenderer() {
    singleton = this;
}

CuRenderer::~CuRenderer() {
    singleton = nullptr;
}

CuRenderer *CuRenderer::get_singleton() {
    return singleton;
}

bool CuRenderer::init(CuWindow *p_window) {
    bool result = false;
    result = device.init(p_window);
    if (result) {
        ENGINE_INFO("Renderer ready");
    }
    return result;
}

void CuRenderer::create_material(const std::vector<std::string>& p_shaders) {
    std::vector<CompiledShaderInfo> shader_infos(p_shaders.size());
    for (int i = 0; i < shader_infos.size(); ++i) {
        if (!shader_compiler.compile_shader(p_shaders[i], &shader_infos[i])) {
            return;
        }
    }
    const PipelineLayoutInfo layout_info = device.generate_pipeline_info(shader_infos);
    device.create_render_pipeline(shader_infos, layout_info);
}

void CuRenderer::draw() {
    device.draw();
}

void CuRenderer::clear() {
    device.clear();
}