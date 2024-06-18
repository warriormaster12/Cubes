#pragma once

#include "shader_compiler.h"
#include <memory>
#include <string>
#include <vector>

class CuWindow;

class RenderPassBase;

class CuGlobalGpuDataManager;

class CuRenderer {
public:
  CuRenderer();
  ~CuRenderer();
  bool init(CuWindow *p_window);
  void create_material(const std::vector<std::string> &p_shaders);
  void add_render_pass(std::unique_ptr<RenderPassBase> p_render_pass);
  void draw();
  void clear();

  static CuRenderer *get_singleton();

private:
  CuWindow *window = nullptr;
  ShaderCompiler shader_compiler;
  std::vector<std::shared_ptr<RenderPassBase>> render_passes;

  static CuRenderer *singleton;
};