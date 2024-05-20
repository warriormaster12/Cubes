#pragma once

class CuWindow;

class CuRenderer {
public:
    bool init(CuWindow *p_window);
    void draw();
    void clear();
private:
    CuWindow *window = nullptr;
};