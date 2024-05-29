#pragma once

#include "shader_compiler.h"

class CuWindow;

class CuRenderer {
public:
    CuRenderer();
    ~CuRenderer();
    bool init(CuWindow *p_window);
    void draw();
    void clear();
    
    static CuRenderer *get_singleton();
private:
    CuWindow *window = nullptr;
    ShaderCompiler shader_compiler;

    static CuRenderer *singleton;
};