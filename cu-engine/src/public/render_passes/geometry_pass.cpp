#include "geometry_pass.h"
#include "render_device/render_device.h"
#include "shader_compiler.h"
#include <vector>
#include <array>
#include <cmath>

Texture draw_texture;
RenderPipeline triangle_pipeline;
Buffer test_buffer;

DescriptorWriter geometry_descriptor_writer = {};

void GeometryPass::init() {
    CuRenderDevice* device = CuRenderDevice::get_singleton();
    if (!device) {
        return;
    }

    VkExtent2D extent = device->get_swapchain_size();

    VkImageUsageFlags draw_image_usage_flags = {};
        draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        draw_image_usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    device->create_texture(
        VK_FORMAT_R16G16B16A16_SFLOAT, 
        {extent.width, extent.height, 1}, 
        draw_image_usage_flags, 
        VMA_MEMORY_USAGE_GPU_ONLY, 
        draw_texture
    );

    std::array<std::string, 2> shaders  = {"assets/shaders/test.vert", "assets/shaders/test.frag"};
    std::vector<CompiledShaderInfo> shader_infos(shaders.size());
    for (int i = 0; i < shader_infos.size(); ++i) {
        ShaderCompiler* shader_compiler = ShaderCompiler::get_singleton();
        if (!shader_compiler) {
            break;
        }
        if (!shader_compiler->compile_shader(shaders[i], &shader_infos[i])) {
            return;
        }
    }
    const PipelineLayoutInfo layout_info = device->generate_pipeline_info(shader_infos);
    triangle_pipeline = device->create_render_pipeline(shader_infos, layout_info, &draw_texture);

    test_buffer = device->create_buffer(sizeof(float) * 4, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    geometry_descriptor_writer.write_buffer(0, test_buffer, 0, sizeof(float) * 4,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    geometry_descriptor_writer.update_set(layout_info.descriptor_sets[0]);
    geometry_descriptor_writer.clear();
};

int frame_number = 0;
void GeometryPass::update() {
    CuRenderDevice* device = CuRenderDevice::get_singleton();
    if (!device) {
        return;
    }
    frame_number++;
    float color[4] = {std::sin(frame_number / 60.f), 0.0f, std::cos(frame_number / 60.f), 1.0f};
    device->write_buffer(color, sizeof(float) * 4, test_buffer);
    device->prepare_image(draw_texture, COLOR);
    device->bind_pipeline(triangle_pipeline);
    device->draw();
    device->submit_image(draw_texture);
}

void GeometryPass::clear() {
    CuRenderDevice* device = CuRenderDevice::get_singleton();
    if (!device) {
        return;
    }
    device->clear_buffer(test_buffer);
    device->clear_texture(draw_texture);
};