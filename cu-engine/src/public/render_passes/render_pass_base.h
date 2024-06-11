#pragma once

class RenderPassBase {
public:
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void clear() = 0;
    virtual ~RenderPassBase() = default;
};