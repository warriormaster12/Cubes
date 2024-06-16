#pragma once

#include "render_pass_base.h"

class CuRenderDevice;

class GeometryPass : public RenderPassBase {
public:
    void init() override;
    void update() override;
    void clear() override;
private:
    CuRenderDevice* device = nullptr;
};