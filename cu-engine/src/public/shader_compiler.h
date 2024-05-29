#pragma once

#include <cstddef>
#include <stdint.h>
#include <string>

enum ShaderStage {
    VERTEX,
    FRAGMENT,
};

struct CompiledShaderInfo {
    size_t code_size;
    const uint32_t *code;
    ShaderStage stage;
};

class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();
    bool compile_shader(const std::string& p_filepath, CompiledShaderInfo *out_info);
    static ShaderCompiler *get_singleton();
private:
    static ShaderCompiler *singleton;

};