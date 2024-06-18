#include "cu-engine.h"

#include "render_passes/geometry_pass.h"

CuEngine::CuEngine(const std::string &p_title) {
  ready = window.init(p_title, 1280, 720);
  ready = renderer.init(&window);
  camera_manager.init();
  renderer.add_render_pass(std::make_unique<GeometryPass>());
}

CuEngine::CuEngine() {
  ready = window.init("test", 1280, 720);
  ready = renderer.init(&window);

  renderer.add_render_pass(std::make_unique<GeometryPass>());
}

bool CuEngine::running() const { return ready && !window.should_close(); }

void CuEngine::clear() {
  renderer.clear();
  window.clear();
}