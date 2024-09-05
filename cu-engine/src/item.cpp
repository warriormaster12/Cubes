#include "item.h"
#include "render_device/render_device.h"

void CuItem::set_id(const std::string p_id) { id = p_id; }

void CuItem::set_position(const glm::vec3 &p_position) {
  position = p_position;
  is_dirty = true;
};

void CuItem::set_rotation(const glm::vec3 &p_rotation) {
  rotation = glm::radians(p_rotation);
  is_dirty = true;
};

void CuItem::set_scale(const glm::vec3 &p_scale) {
  scale = p_scale;
  is_dirty = true;
};

void CuItem::update(double p_delta) {
  if (is_dirty) {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    transform = glm::mat4{{
                              scale.x * (c1 * c3 + s1 * s2 * s3),
                              scale.x * (c2 * s3),
                              scale.x * (c1 * s2 * s3 - c3 * s1),
                              0.0f,
                          },
                          {
                              scale.y * (c3 * s1 * s2 - c1 * s3),
                              scale.y * (c2 * c3),
                              scale.y * (c1 * c3 * s2 + s1 * s3),
                              0.0f,
                          },
                          {
                              scale.z * (c2 * s1),
                              scale.z * (-s2),
                              scale.z * (c1 * c2),
                              0.0f,
                          },
                          {position.x, position.y, position.z, 1.0f}};
  }

  // we reset the flag later for renderables
  if (!(item_type & CuItemType::RENDERABLE))
    is_dirty = false;
}

CuItemManager *CuItemManager::singleton = nullptr;

CuItemManager::CuItemManager() { singleton = this; }

void CuItemManager::add_item(CuItem item) {

  if ((item.get_type() & CuItemType::RENDERABLE)) {
    if (renderables_count == 0) {
      CuRenderDevice *device = CuRenderDevice::get_singleton();
      if (device) {
        {
          // create vertex buffer
          {
            cube_vertex_buffer =
                device->create_buffer(sizeof(Vertex) * cube_vertices.size(),
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                      VMA_MEMORY_USAGE_GPU_ONLY);

            Buffer staging_buffer = device->create_buffer(
                sizeof(Vertex) * cube_vertices.size(),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

            device->write_buffer(cube_vertices.data(),
                                 sizeof(Vertex) * cube_vertices.size(),
                                 staging_buffer);
            device->immediate_submit([&]() {
              device->copy_buffer(staging_buffer, cube_vertex_buffer,
                                  sizeof(Vertex) * cube_vertices.size(), 0, 0);
            });

            device->clear_buffer(staging_buffer);
          }
        }
        // create index buffer
        {
          cube_index_buffer =
              device->create_buffer(sizeof(uint16_t) * cube_indices.size(),
                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    VMA_MEMORY_USAGE_GPU_ONLY);

          Buffer staging_buffer = device->create_buffer(
              sizeof(uint16_t) * cube_indices.size(),
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

          device->write_buffer(cube_indices.data(),
                               sizeof(uint16_t) * cube_indices.size(),
                               staging_buffer);
          device->immediate_submit([&]() {
            device->copy_buffer(staging_buffer, cube_index_buffer,
                                sizeof(uint16_t) * cube_indices.size(), 0, 0);
          });

          device->clear_buffer(staging_buffer);
        }
      }
    }
    renderables_count += 1;
  }

  items.push_back(item);
}

CuItemManager *CuItemManager::get_singleton() { return singleton; }

CuItem *CuItemManager::get_item(const std::string &id) {
  for (int i = 0; i < items.size(); ++i) {
    if (items[i].get_id() == id) {
      return &items[i];
    }
  }

  return nullptr;
}

std::vector<CuItem *> CuItemManager::get_items_by_type(CuItemType p_type) {
  std::vector<CuItem *> out_items;
  for (int i = 0; i < items.size(); ++i) {
    if (items[i].get_type() & p_type) {
      out_items.push_back(&items[i]);
    }
  }
  return out_items;
}

void CuItemManager::update_items(double p_delta) {
  for (int i = 0; i < items.size(); ++i) {
    CuItem &item = items[i];
    item.update(p_delta);
  }
}

void CuItemManager::draw_items() {
  CuRenderDevice *device = CuRenderDevice::get_singleton();
  if (!device || cube_vertex_buffer.buffer == VK_NULL_HANDLE) {
    return;
  }
  std::vector<CuItem *> renderables = get_items_by_type(CuItemType::RENDERABLE);
  const int instance_count = renderables.size();

  device->bind_vertex_buffer(0, 1, {cube_vertex_buffer}, {0});
  if (cube_index_buffer.buffer == VK_NULL_HANDLE) {
    device->draw(cube_vertices.size(), instance_count, 0, 0);
  } else {
    device->bind_index_buffer(cube_index_buffer, 0);
    device->draw_indexed(static_cast<uint16_t>(cube_indices.size()),
                         instance_count, 0, 0, 0);
  }

  for (int i = 0; i < renderables.size(); ++i) {
    renderables[i]->reset_dirty_state();
  }
}

void CuItemManager::clear_renderable_items() {
  CuRenderDevice *device = CuRenderDevice::get_singleton();
  if (!device) {
    return;
  }
  device->clear_buffer(cube_vertex_buffer);
  device->clear_buffer(cube_index_buffer);
}