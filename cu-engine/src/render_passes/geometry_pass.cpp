#include "geometry_pass.h"
#include "camera.h"
#include "item.h"
#include "render_device/render_device.h"
#include "shader_compiler.h"
#include <array>

#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/transform.hpp>

Texture color_texture;
Texture depth_texture;
RenderPipeline triangle_pipeline;
Buffer test_buffer;
Buffer transform_buffer;

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
    transform_buffer = device->create_buffer(sizeof(glm::mat4) * 1000,
                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                             VMA_MEMORY_USAGE_CPU_TO_GPU);
    test_buffer = device->create_buffer(sizeof(float) * 4,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
    geometry_descriptor_writer.write_buffer(0, transform_buffer, 0,
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
  CuItemManager *item_manager = CuItemManager::get_singleton();
  frame_number++;

  float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  if (item_manager) {
    std::vector<CuItem *> renderables =
        item_manager->get_items_by_type(CuItemType::RENDERABLE);

    const int count = renderables.size();
    bool update_transforms = false;
    for (int i = 0; i < count; ++i) {
      if (renderables[i]->get_dirty_state()) {
        update_transforms = true;
        break;
      }
    }

    std::vector<glm::mat4> transforms = {};
    transforms.resize(count);
    if (update_transforms) {
      for (int i = 0; i < count; ++i) {
        transforms[i] = renderables[i]->get_transform();
      }

      device->write_buffer(transforms.data(), sizeof(glm::mat4) * count,
                           transform_buffer);
    }
  }
  device->write_buffer(color, sizeof(float) * 4, test_buffer);

  CuRenderAttachmentBuilder builder;
  builder.add_color_attachment({0.0, 0.0, 0.0, 1.0});
  builder.add_depth_attachment();
  CuRenderAttachemnts render_attachments = builder.build();
  device->prepare_image(render_attachments, &color_texture, &depth_texture);
  device->bind_pipeline(triangle_pipeline);
  device->bind_descriptor(triangle_pipeline, 0);
  device->bind_descriptor(triangle_pipeline, 1);
  if (item_manager) {
    item_manager->draw_items();
  }
  device->submit_image(color_texture);
}

void GeometryPass::clear() {
  if (!device) {
    return;
  }
  device->clear_buffer(test_buffer);
  device->clear_buffer(transform_buffer);
  device->clear_texture(color_texture);
  device->clear_texture(depth_texture);
  triangle_pipeline.clear(device->get_raw_device());
};