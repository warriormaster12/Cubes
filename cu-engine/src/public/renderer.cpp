#include "renderer.h"
#include "render_device/render_device.h"
#include "window.h"
#include "logger.h"

CuRenderDevice device;

Texture draw_texture;
RenderPipeline triangle_pipeline;

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
    if (!device.init(p_window)) {
        ENGINE_INFO("Renderer ready");
        return false;
    }

    VkExtent2D extent = device.get_swapchain_size();

    VkImageUsageFlags draw_image_usage_flags = {};
        draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    device.create_texture(
        VK_FORMAT_R16G16B16A16_SFLOAT, 
        {extent.width, extent.height, 1}, 
        draw_image_usage_flags, 
        VMA_MEMORY_USAGE_GPU_ONLY, 
        draw_texture
    );
    return true;
}

void CuRenderer::create_material(const std::vector<std::string>& p_shaders) {
    std::vector<CompiledShaderInfo> shader_infos(p_shaders.size());
    for (int i = 0; i < shader_infos.size(); ++i) {
        if (!shader_compiler.compile_shader(p_shaders[i], &shader_infos[i])) {
            return;
        }
    }
    const PipelineLayoutInfo layout_info = device.generate_pipeline_info(shader_infos);
    triangle_pipeline = device.create_render_pipeline(shader_infos, layout_info, &draw_texture);
}

void CuRenderer::draw() {
    device.begin_recording();
    device.prepare_image(draw_texture, COLOR);
    device.bind_pipeline(triangle_pipeline);
    device.draw();
    device.submit_image(draw_texture);
    device.finish_recording();
}

void CuRenderer::clear() {
    device.stop_rendering();
    device.clear_texture(draw_texture);
    device.clear();
}