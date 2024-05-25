#pragma once

#include "shader_compiler.h"

class CuWindow;

class CuRenderer {
public:
    bool init(CuWindow *p_window);
    void draw();
    void clear();
private:
    CuWindow *window = nullptr;
    ShaderCompiler shader_compiler;
};