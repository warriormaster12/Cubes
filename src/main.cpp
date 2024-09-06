#include <camera.h>
#include <cu-engine.h>
#include <item.h>
#include <renderer.h>
#include <shader_compiler.h>

int main() {
  CuEngine engine = CuEngine("Cubes");
  ShaderCompiler *s_compiler = ShaderCompiler::get_singleton();
  Camera *main_camera = CameraManager::get_singleton()->create_camera();
  CuItemManager item_manager = CuItemManager();
  item_manager.add_root(std::make_shared<CuItem>("root", CuItemType::NONE));
  CuItem *root = item_manager.get_item("root");
  root->add_child(CuItem("cube", CuItemType::RENDERABLE));
  root->add_child(CuItem("floor", CuItemType::RENDERABLE));
  CuItem *cube = item_manager.get_item("cube");
  cube->add_child(CuItem("cube1", CuItemType::RENDERABLE));
  CuItem *cube1 = item_manager.get_item("cube1");

  CuItem *floor = item_manager.get_item("floor");

  floor->set_scale(glm::vec3(10.0, 10.0, 0.1f));
  cube->set_position(glm::vec3(0.0, 0.0, 5.0));
  cube1->set_position(glm::vec3(3.0, 0.0, 0.0));
  float angle = 0.0;
  while (engine.running()) {
    engine.window.poll_events();
    angle += 1.0;
    cube->set_rotation(glm::vec3(0.0, 0.0, angle));
    cube1->set_rotation(glm::vec3(angle * 2, 0, angle * 2));
    item_manager.update_items(1.0 / 60.0);
    if (main_camera) {
      main_camera->update();
    }
    CuRenderer::get_singleton()->draw();
  }
  engine.clear();
  return 0;
}
