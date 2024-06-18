#include <camera.h>
#include <cu-engine.h>
#include <renderer.h>
#include <shader_compiler.h>

int main() {
  CuEngine engine = CuEngine("Cubes");
  ShaderCompiler *s_compiler = ShaderCompiler::get_singleton();
  Camera *main_camera = CameraManager::get_singleton()->create_camera();
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
