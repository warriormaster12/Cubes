#include <cu-engine.h>


int main() {
    CuEngine engine;
    while (engine.running()) {
        engine.window.poll_events();
        engine.renderer.draw();
    }
    engine.clear();
    return 0;
}
