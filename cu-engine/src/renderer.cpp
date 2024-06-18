#include "renderer.h"

#include "camera.h"
#include "logger.h"
#include "render_device/render_device.h"
#include "render_passes/render_pass_base.h"
#include "window.h"

CuRenderDevice device;

CuRenderer *CuRenderer::singleton = nullptr;

CuRenderer::CuRenderer() { singleton = this; }

CuRenderer::~CuRenderer() { singleton = nullptr; }

CuRenderer *CuRenderer::get_singleton() { return singleton; }

bool CuRenderer::init(CuWindow *p_window) {
  if (!device.init(p_window)) {
    ENGINE_INFO("Renderer ready");
    return false;
  }
  return true;
}

void CuRenderer::create_material(const std::vector<std::string> &p_shaders) {}

void CuRenderer::add_render_pass(
    std::unique_ptr<RenderPassBase> p_render_pass) {
  render_passes.push_back(std::move(p_render_pass));
  render_passes.back()->init();
};

void CuRenderer::draw() {
  device.begin_recording();
  CameraManager *camera_manager = CameraManager::get_singleton();
  if (camera_manager) {
    camera_manager->update_active_camera();
  }

  for (int i = 0; i < render_passes.size(); ++i) {
    render_passes[i]->update();
  }
  device.finish_recording();
}

void CuRenderer::clear() {
  device.stop_rendering();
  for (int i = 0; i < render_passes.size(); ++i) {
    render_passes[i]->clear();
  }
  CameraManager *camera_manager = CameraManager::get_singleton();
  if (camera_manager) {
    camera_manager->clear();
  }
  device.clear();
}