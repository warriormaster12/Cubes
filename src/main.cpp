#include <camera.h>
#include <chrono>
#include <cu-engine.h>
#include <item.h>
#include <physics-server.h>
#include <renderer.h>

int main() {
  CuEngine engine = CuEngine("Cubes");
  CuPhysicsServer *physics = CuPhysicsServer::get_singleton();
  Camera *main_camera = CameraManager::get_singleton()->create_camera();
  CuItemManager item_manager = CuItemManager();
  item_manager.add_root(std::make_shared<CuItem>("root", CuItemType::NONE));
  CuItem *root = item_manager.get_item("root");
  root->add_child(CuItem("cube", CuItemType::RENDERABLE));
  root->add_child(
      CuItem("floor", CuItemType::RENDERABLE | CuItemType::STATIC_BODY));
  root->add_child(
      CuItem("rigid_cube", CuItemType::RENDERABLE | CuItemType::RIGID_BODY));
  CuItem *cube = item_manager.get_item("cube");
  cube->add_child(CuItem("cube1", CuItemType::RENDERABLE));
  cube->add_child(CuItem("cube2", CuItemType::RENDERABLE));
  CuItem *cube1 = item_manager.get_item("cube1");
  CuItem *cube2 = item_manager.get_item("cube2");

  CuItem *floor = item_manager.get_item("floor");
  CuItem *rigid_cube = item_manager.get_item("rigid_cube");
  rigid_cube->set_position(glm::vec3(0.0, 0.0, 7.0));

  floor->set_scale(glm::vec3(10.0, 10.0, 0.1f));
  cube->set_position(glm::vec3(0.0, 0.0, 5.0));
  cube1->set_position(glm::vec3(3.0, 0.0, 0.0));
  cube2->set_position(glm::vec3(-3.0, 0.0, 0.0));
  float angle = 0.0;
  double delta = 0.0;

  while (engine.running()) {
    engine.window.poll_events();
    auto start = std::chrono::high_resolution_clock::now();
    angle += 20.0 * delta;
    cube->set_rotation(glm::vec3(0.0, 0.0, angle));
    cube1->set_rotation(glm::vec3(angle * 2, 0, angle * 2));
    cube2->set_rotation(glm::vec3(angle * 2, 0, angle * 2));
    item_manager.update_items();
    if (physics) {
      physics->update_physics(1.0 / 60.0);
    }
    if (main_camera) {
      main_camera->update();
    }
    CuRenderer::get_singleton()->draw();
    auto end = std::chrono::high_resolution_clock::now();
    double diff =
        std::chrono::duration<double, std::milli>(end - start).count();
    delta = diff * 0.001;
  }
  engine.clear();
  return 0;
}
