#include <cu-engine.h>
#include <shader_compiler.h>

int main() {
    CuEngine engine;
    ShaderCompiler *s_compiler = ShaderCompiler::get_singleton();
    CompiledShaderInfo out_info = {};
    if (s_compiler->compile_shader("assets/shaders/test.vertx", VERTEX, &out_info)) {
        
    }
    while (engine.running()) {
        engine.window.poll_events();
        engine.renderer.draw();
    }
    engine.clear();
    return 0;
}
