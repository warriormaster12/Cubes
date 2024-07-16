#include <camera.h>
#include <cu-engine.h>
#include <renderer.h>
#include <shader_compiler.h>
#include "entity/entity.h"

int main() {
  CuEngine engine = CuEngine("Cubes");
  ShaderCompiler *s_compiler = ShaderCompiler::get_singleton();
  Camera *main_camera = CameraManager::get_singleton()->create_camera();
  std::shared_ptr<Entity> root = Entity::create();
  root->id = "root";

  std::shared_ptr<Entity> child1 = Entity::create();
  child1->id = "child1";

  std::shared_ptr<Entity> child2 = Entity::create();
  child2->id = "child2";

  root->add_child(child1);
  root->add_child(child2);

  root->ready();

  root->free();
  while (engine.running()) {
    engine.window.poll_events();
    if (main_camera) {
      main_camera->update();
    }
    CuRenderer::get_singleton()->draw();
  }
  engine.clear();
  return 0;
}
