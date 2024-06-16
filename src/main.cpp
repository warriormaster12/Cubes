#include <cu-engine.h>
#include <renderer.h>
#include <camera.h>
#include <shader_compiler.h>

int main() {
    CuEngine engine = CuEngine("Cubes");
    ShaderCompiler *s_compiler = ShaderCompiler::get_singleton();

    Camera camera;

    while (engine.running()) {
        engine.window.poll_events();
        camera.update();
        CuRenderer::get_singleton()->draw();
    }
    engine.clear();
    return 0;
}
