#pragma once
#include <stdint.h>
#include <string>
#include <vector>

enum ShaderStage {
  VERTEX,
  FRAGMENT,
};

struct CompiledShaderInfo {
  std::vector<uint32_t> buffer;
  ShaderStage stage;
};

class ShaderCompiler {
public:
  ShaderCompiler();
  ~ShaderCompiler();
  bool compile_shader(const std::string &p_filepath,
                      CompiledShaderInfo *out_info);
  static ShaderCompiler *get_singleton();

private:
  static ShaderCompiler *singleton;
};