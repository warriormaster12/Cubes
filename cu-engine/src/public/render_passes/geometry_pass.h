#pragma once

#include "render_pass_base.h"

class GeometryPass : public RenderPassBase {
public:
    void init() override;
    void update() override;
    void clear() override;
};