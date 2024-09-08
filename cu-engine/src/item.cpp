#include "item.h"
#include "render_device/render_device.h"

CuItem::CuItem(const std::string p_id, const int p_item_type) {
  id = p_id;
  item_type = (CuItemType)p_item_type;

  CuPhysicsServer *physics = CuPhysicsServer::get_singleton();

  if (physics) {
    if ((item_type & CuItemType::STATIC_BODY) == CuItemType::STATIC_BODY ||
        (item_type & CuItemType::RIGID_BODY) == CuItemType::RIGID_BODY) {
      shape = physics->create_box_shape(glm::vec3(scale.x, scale.y, scale.z));
    }

    if ((item_type & CuItemType::STATIC_BODY) == CuItemType::STATIC_BODY) {
      bt_transform.setIdentity();
      collision_object = physics->create_static_body(bt_transform, shape);
    } else if ((item_type & CuItemType::RIGID_BODY) == CuItemType::RIGID_BODY) {
      bt_transform.setIdentity();
      body = physics->create_rigid_body(5.0f, bt_transform, shape);
    }
  }
}

CuItem::~CuItem() {
  CuPhysicsServer *physics = CuPhysicsServer::get_singleton();
  // if (physics && shape) {
  //   physics->remove_collision_shape(shape);
  // }
}

void CuItem::set_id(const std::string p_id) { id = p_id; }

void CuItem::set_position(const glm::vec3 &p_position) {
  position = p_position;
  if (body) {
    bt_transform.setOrigin(btVector3(position.x, position.y, position.z));
    body->setWorldTransform(bt_transform);
  }
  is_dirty = true;
};

void CuItem::set_rotation(const glm::vec3 &p_rotation) {
  rotation = glm::radians(p_rotation);
  is_dirty = true;
};

void CuItem::set_scale(const glm::vec3 &p_scale) {
  scale = p_scale;
  if ((item_type & CuItemType::STATIC_BODY) == CuItemType::STATIC_BODY ||
      (item_type & CuItemType::RIGID_BODY) == CuItemType::RIGID_BODY) {
    CuPhysicsServer *physics = CuPhysicsServer::get_singleton();
    if (physics) {
      switch (shape->getShapeType()) {
      case BOX_SHAPE_PROXYTYPE:
        static_cast<btBoxShape *>(shape)->setImplicitShapeDimensions(
            btVector3(scale.x, scale.y, scale.z));
        break;
      default:
        break;
      }
    }
  }
  is_dirty = true;
};

void CuItem::add_child(CuItem p_item) {
  p_item.parent = this;
  children.push_back(std::move(p_item));
}

void CuItem::update() {
  if (body && !body->wantsSleeping()) {
    position =
        glm::vec3(position.x + body->getLinearVelocity().getX() * 1 / 60,
                  position.y + body->getLinearVelocity().getY() * 1 / 60,
                  position.z + body->getLinearVelocity().getZ() * 1 / 60);

    is_dirty = true;
  }

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

    if (parent) {
      transform = parent->transform * transform;
    }
  }

  for (int i = 0; i < children.size(); ++i) {
    children[i].update();
  }
  // we reset the flag later for renderables
  if (!((item_type & CuItemType::RENDERABLE) == CuItemType::RENDERABLE))
    is_dirty = false;
}

CuItemManager *CuItemManager::singleton = nullptr;

CuItemManager::CuItemManager() { singleton = this; }
CuItemManager::~CuItemManager() { singleton = nullptr; }

void CuItemManager::add_root(std::shared_ptr<CuItem> p_item) {
  if (root) {
    return;
  }
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
      cube_index_buffer = device->create_buffer(
          sizeof(uint16_t) * cube_indices.size(),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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

  root = p_item;
}

CuItemManager *CuItemManager::get_singleton() { return singleton; }

CuItem *find_item_in_node(CuItem *current_item, const std::string &p_id) {
  CuItem *found_item = nullptr;
  for (int i = 0; i < current_item->get_child_count(); ++i) {
    CuItem *item = current_item->get_child(i);
    if (!item) {
      continue;
    }
    if (item->get_id() == p_id) {
      return item;
    } else {
      found_item = find_item_in_node(item, p_id);
      if (found_item) {
        break;
      }
    }
  }
  return found_item;
}

std::vector<CuItem *> find_items_of_type_in_node(CuItem *current_item,
                                                 const CuItemType &p_type) {
  std::vector<CuItem *> found_items;
  for (int i = 0; i < current_item->get_child_count(); ++i) {
    CuItem *item = current_item->get_child(i);
    if ((item->get_type() & p_type) == p_type) {
      found_items.push_back(item);
    }
    std::vector<CuItem *> temp_list = find_items_of_type_in_node(item, p_type);
    found_items.insert(found_items.end(), temp_list.begin(), temp_list.end());
  }
  return found_items;
}

CuItem *CuItemManager::get_item(const std::string &p_id) {
  if (!root) {
    return nullptr;
  } else if (root && root->get_id() == p_id) {
    return root.get();
  }

  return find_item_in_node(root.get(), p_id);
}

std::vector<CuItem *> CuItemManager::get_items_by_type(CuItemType p_type) {

  return find_items_of_type_in_node(root.get(), p_type);
}

void CuItemManager::update_items() { root->update(); }

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