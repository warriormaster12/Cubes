#pragma once

#include "render_pass_base.h"

class CuRenderDevice;
class CameraManager;

class GeometryPass : public RenderPassBase {
public:
  void init() override;
  void update() override;
  void clear() override;

private:
  CuRenderDevice *device = nullptr;
  CameraManager *camera_manager = nullptr;
};