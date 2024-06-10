#include <cu-engine.h>
#include <renderer.h>
#include <shader_compiler.h>

int main() {
    CuEngine engine = CuEngine("Cubes");
    ShaderCompiler *s_compiler = ShaderCompiler::get_singleton();
    CompiledShaderInfo out_info = {};
    CuRenderer::get_singleton()->create_material({"assets/shaders/test.vert", "assets/shaders/test.frag"});
    while (engine.running()) {
        engine.window.poll_events();
        CuRenderer::get_singleton()->draw();
    }
    engine.clear();
    return 0;
}
