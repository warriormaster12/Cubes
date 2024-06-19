#include "geometry_pass.h"
#include "camera.h"
#include "render_device/render_device.h"
#include "shader_compiler.h"
#include <array>
#include <cmath>

#include <vector>

Texture draw_texture;
RenderPipeline triangle_pipeline;
Buffer test_buffer;
Buffer test_vertex_buffer;

std::vector<glm::vec2> vertices = {{-0.5f, -0.5f}, {0.5f, -0.5f},
                                   {0.5f, 0.5f},   {-0.5f, -0.5f},
                                   {0.5f, 0.5f},   {-0.5f, 0.5f}};

DescriptorWriter geometry_descriptor_writer = {};

void GeometryPass::init() {
  device = CuRenderDevice::get_singleton();
  camera_manager = CameraManager::get_singleton();
  if (!device || !camera_manager) {
    return;
  }

  VkExtent2D extent = device->get_swapchain_size();

  VkImageUsageFlags draw_image_usage_flags = {};
  draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  device->create_texture(
      VK_FORMAT_R16G16B16A16_SFLOAT, {extent.width, extent.height, 1},
      draw_image_usage_flags, VMA_MEMORY_USAGE_GPU_ONLY, draw_texture);

  std::array<std::string, 2> shaders = {"assets/shaders/test.vert",
                                        "assets/shaders/test.frag"};
  std::vector<CompiledShaderInfo> shader_infos(shaders.size());
  for (int i = 0; i < shader_infos.size(); ++i) {
    ShaderCompiler *shader_compiler = ShaderCompiler::get_singleton();
    if (!shader_compiler) {
      break;
    }
    if (!shader_compiler->compile_shader(shaders[i], &shader_infos[i])) {
      return;
    }
  }

  {
    test_vertex_buffer = device->create_buffer(
        sizeof(glm::vec2) * vertices.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    Buffer staging_buffer = device->create_buffer(
        sizeof(glm::vec2) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    device->write_buffer(vertices.data(), sizeof(glm::vec2) * vertices.size(),
                         staging_buffer);
    device->immediate_submit([&]() {
      device->copy_buffer(staging_buffer, test_vertex_buffer,
                          sizeof(glm::vec2) * vertices.size(), 0, 0);
    });

    device->clear_buffer(staging_buffer);
  }

  triangle_pipeline =
      device->create_render_pipeline(shader_infos, &draw_texture);
  {
    // set 0
    geometry_descriptor_writer.write_buffer(
        0, camera_manager->get_camera_buffer(), 0, sizeof(glm::mat4) * 2,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    geometry_descriptor_writer.update_set(triangle_pipeline.sets[0]);
    geometry_descriptor_writer.update_set(triangle_pipeline.sets[2]);
    geometry_descriptor_writer.clear();
  }
  {
    // set 1
    test_buffer = device->create_buffer(sizeof(float) * 4,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
    geometry_descriptor_writer.write_buffer(0, test_buffer, 0,
                                            sizeof(float) * 4,
                                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    geometry_descriptor_writer.update_set(triangle_pipeline.sets[1]);
    geometry_descriptor_writer.update_set(triangle_pipeline.sets[3]);
    geometry_descriptor_writer.clear();
  }
};

int frame_number = 0;
void GeometryPass::update() {
  if (!device) {
    return;
  }
  frame_number++;
  float color[4] = {std::sin(frame_number / 60.f), 0.0f,
                    std::cos(frame_number / 60.f), 1.0f};
  device->write_buffer(color, sizeof(float) * 4, test_buffer);

  CuRenderAttachmentBuilder builder;
  builder.add_color_attachment({0.0, 1.0, 0.0, 1.0});
  CuRenderAttachemnts render_attachments = builder.build();
  device->prepare_image(draw_texture, render_attachments);
  device->bind_pipeline(triangle_pipeline);
  device->bind_descriptor(triangle_pipeline, 0);
  device->bind_descriptor(triangle_pipeline, 1);
  device->bind_vertex_buffer(0, 1, {test_vertex_buffer}, {0});
  device->draw(vertices.size(), 1, 0, 0);
  device->submit_image(draw_texture);
}

void GeometryPass::clear() {
  if (!device) {
    return;
  }
  device->clear_buffer(test_buffer);
  device->clear_buffer(test_vertex_buffer);
  device->clear_texture(draw_texture);
  triangle_pipeline.clear(device->get_raw_device());
};