// Solaris History - ventana Win32 y contexto OpenGL
#pragma once
#include "core.hpp"
#include <windows.h>

namespace sol {

struct Platform {
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC glrc = nullptr;
    HINSTANCE inst = nullptr;
    int w = 1600, h = 900;
    bool running = true;

    bool mouseDown[3] = {false, false, false};
    bool mousePressed[3] = {false, false, false};
    bool mouseReleased[3] = {false, false, false};
    float mx = 0, my = 0, pmx = 0, pmy = 0;
    float wheel = 0;
    bool keys[256] = {false};
    bool keyPressed[256] = {false};
    double startTime = 0;

    bool init(HINSTANCE instance, int width, int height, const wchar_t* title);
    void pump();
    void swap();
    void shutdown();
    double now() const;
    void setCursorHand(bool hand);
    bool hasFocus() const { return GetFocus() == hwnd; }
};

} // namespace sol
