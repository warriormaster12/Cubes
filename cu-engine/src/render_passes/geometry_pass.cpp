#include "geometry_pass.h"
#include "camera.h"
#include "render_device/render_device.h"
#include "shader_compiler.h"
#include <array>
#include <cmath>

#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/transform.hpp>

Texture color_texture;
Texture depth_texture;
RenderPipeline triangle_pipeline;
Buffer test_buffer;
Buffer test_vertex_buffer;
Buffer test_index_buffer;
Buffer test_transform_buffer;

struct Vertex {
  glm::vec3 position;
  glm::vec3 normals;
};

std::vector<Vertex> vertices = {
    // Front face
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}, // 0
    {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},  // 1
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},   // 2
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},  // 3
    // Back face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}}, // 4
    {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},  // 5
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},   // 6
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},  // 7
    // Top face
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}}, // 7
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},  // 6
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},   // 2
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},  // 3
    // Bottom face
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}}, // 4
    {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}},  // 5
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},   // 1
    {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},  // 0
    // Right face
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}, // 5
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},  // 6
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},   // 2
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},  // 1
    // Left face
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}}, // 4
    {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},  // 7
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},   // 3
    {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},  // 0
};

// Define the indices for the 12 triangles that make up the cube
std::vector<uint16_t> indices = {
    // Front face
    0, 1, 2, 2, 3, 0,
    // Back face
    4, 5, 6, 6, 7, 4,
    // Top face
    8, 9, 10, 10, 11, 8,
    // Bottom face
    12, 13, 14, 14, 15, 12,
    // Right face
    16, 17, 18, 18, 19, 16,
    // Left face
    20, 21, 22, 22, 23, 20};

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
      draw_image_usage_flags, VMA_MEMORY_USAGE_GPU_ONLY, color_texture);

  device->create_texture(VK_FORMAT_D32_SFLOAT, {extent.width, extent.height, 1},
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         VMA_MEMORY_USAGE_GPU_ONLY, depth_texture);

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

  // Create and allocate vertex buffer
  {
    test_vertex_buffer = device->create_buffer(
        sizeof(Vertex) * vertices.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    Buffer staging_buffer = device->create_buffer(
        sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    device->write_buffer(vertices.data(), sizeof(Vertex) * vertices.size(),
                         staging_buffer);
    device->immediate_submit([&]() {
      device->copy_buffer(staging_buffer, test_vertex_buffer,
                          sizeof(Vertex) * vertices.size(), 0, 0);
    });

    device->clear_buffer(staging_buffer);
  }

  // Create and allocate index buffer
  {
    test_index_buffer = device->create_buffer(
        sizeof(uint16_t) * indices.size(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    Buffer staging_buffer = device->create_buffer(
        sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    device->write_buffer(indices.data(), sizeof(uint16_t) * indices.size(),
                         staging_buffer);
    device->immediate_submit([&]() {
      device->copy_buffer(staging_buffer, test_index_buffer,
                          sizeof(uint16_t) * indices.size(), 0, 0);
    });

    device->clear_buffer(staging_buffer);
  }

  triangle_pipeline = device->create_render_pipeline(
      shader_infos, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL, {color_texture},
      depth_texture.format);
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
    test_transform_buffer = device->create_buffer(
        sizeof(glm::mat4) * 1000, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    test_buffer = device->create_buffer(sizeof(float) * 4,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
    geometry_descriptor_writer.write_buffer(0, test_transform_buffer, 0,
                                            sizeof(glm::mat4) * 1000,
                                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    geometry_descriptor_writer.write_buffer(1, test_buffer, 0,
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

  glm::mat4 transforms[7];
  glm::vec3 positions[7] = {glm::vec3(0.0, 0, 0.0), glm::vec3(0, 0, 3),
                            glm::vec3(0, 0, -3),    glm::vec3(3, 0, 0),
                            glm::vec3(-3, 0, 0),    glm::vec3(0, -3, 0),
                            glm::vec3(0, 3, 0)};
  for (int i = 0; i < 7; ++i) {
    transforms[i] =
        glm::translate(glm::mat4(1.0f), positions[i]) *

        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f) * frame_number / 60.f,
                    glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::vec3(1.0, 1.0, 1.0));
  }

  float color[4] = {std::sin(frame_number / 60.f), 0.0f,
                    std::cos(frame_number / 60.f), 1.0f};
  device->write_buffer(transforms, sizeof(glm::mat4) * 7,
                       test_transform_buffer);
  device->write_buffer(color, sizeof(float) * 4, test_buffer);

  CuRenderAttachmentBuilder builder;
  builder.add_color_attachment({0.0, 0.0, 0.0, 1.0});
  builder.add_depth_attachment();
  CuRenderAttachemnts render_attachments = builder.build();
  device->prepare_image(render_attachments, &color_texture, &depth_texture);
  device->bind_pipeline(triangle_pipeline);
  device->bind_descriptor(triangle_pipeline, 0);
  device->bind_descriptor(triangle_pipeline, 1);
  device->bind_vertex_buffer(0, 1, {test_vertex_buffer}, {0});
  device->bind_index_buffer(test_index_buffer, 0);
  device->draw_indexed(static_cast<uint16_t>(indices.size()), 7, 0, 0, 0);
  device->submit_image(color_texture);
}

void GeometryPass::clear() {
  if (!device) {
    return;
  }
  device->clear_buffer(test_buffer);
  device->clear_buffer(test_vertex_buffer);
  device->clear_buffer(test_index_buffer);
  device->clear_buffer(test_transform_buffer);
  device->clear_texture(color_texture);
  device->clear_texture(depth_texture);
  triangle_pipeline.clear(device->get_raw_device());
};