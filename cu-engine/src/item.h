#pragma once
#include "physics-server.h"
#include "render_device/utils.h"
#include <string>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/transform.hpp>

enum CuItemType {
  NONE,
  RENDERABLE = 1 << 0,
  STATIC_BODY = 1 << 1,
  RIGID_BODY = 1 << 2
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 normals;
};

class CuItem {
public:
  CuItem(const std::string p_id, const int p_item_type);
  ~CuItem();

  void set_id(const std::string p_id);
  std::string get_id() const { return id; }

  void set_position(const glm::vec3 &p_position);
  glm::vec3 get_position() const { return position; }

  void set_rotation(const glm::vec3 &p_rotation);
  glm::vec3 get_rotation() const { return rotation; }

  void set_scale(const glm::vec3 &p_scale);
  glm::vec3 get_scale() const { return scale; }

  glm::mat4 get_transform() const { return transform; }

  CuItemType get_type() const { return item_type; }

  void reset_dirty_state() { is_dirty = false; }
  bool get_dirty_state() const { return is_dirty; }

  void update();

  void add_child(CuItem p_item);
  std::vector<CuItem> &get_children() { return children; }
  CuItem *get_child(const int idx) {
    if (idx < 0 || idx >= children.size()) {
      return nullptr;
    }
    return &children[idx];
  }

  size_t get_child_count() const { return children.size(); }

private:
  std::string id;
  glm::vec3 position = glm::vec3(0.0);
  glm::vec3 rotation = glm::vec3(0.0);
  glm::vec3 scale = glm::vec3(1.0);
  glm::mat4 transform;
  CuItemType item_type = NONE;
  CuItem *parent = nullptr;
  std::vector<CuItem> children;
  btCollisionShape *shape = nullptr;
  btCollisionObject *collision_object = nullptr;
  btRigidBody *body = nullptr;
  bool is_dirty = true;
};

class CuItemManager {
public:
  CuItemManager();
  ~CuItemManager();
  void add_root(std::shared_ptr<CuItem> p_item);
  CuItem *get_item(const std::string &p_id);
  std::vector<CuItem *> get_items_by_type(CuItemType p_type);

  void update_items();

  void draw_items();

  void clear_renderable_items();

  static CuItemManager *get_singleton();

private:
  std::shared_ptr<CuItem> root;
  static CuItemManager *singleton;
  Buffer cube_vertex_buffer;
  Buffer cube_index_buffer;
  Buffer transforms_buffer;

  std::vector<Vertex> cube_vertices = {
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
  std::vector<uint16_t> cube_indices = {
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
};