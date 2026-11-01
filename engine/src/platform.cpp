#include "solaris/platform.hpp"
#include <GL/gl.h>
#include <windowsx.h>

namespace sol {

static Platform* g_platform = nullptr;

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    Platform* p = g_platform;
    if (!p) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
        case WM_CLOSE:
            p->running = false;
            return 0;
        case WM_DESTROY:
            p->running = false;
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            int nw = LOWORD(lp), nh = HIWORD(lp);
            if (nw > 0 && nh > 0) { p->w = nw; p->h = nh; }
            return 0;
        }
        case WM_MOUSEMOVE:
            p->mx = (float)GET_X_LPARAM(lp);
            p->my = (float)GET_Y_LPARAM(lp);
            return 0;
        case WM_LBUTTONDOWN:
            p->mouseDown[0] = true; p->mousePressed[0] = true;
            p->mx = (float)GET_X_LPARAM(lp); p->my = (float)GET_Y_LPARAM(lp);
            SetCapture(hwnd);
            return 0;
        case WM_LBUTTONUP:
            p->mouseDown[0] = false; p->mouseReleased[0] = true;
            p->mx = (float)GET_X_LPARAM(lp); p->my = (float)GET_Y_LPARAM(lp);
            ReleaseCapture();
            return 0;
        case WM_RBUTTONDOWN:
            p->mouseDown[1] = true; p->mousePressed[1] = true;
            return 0;
        case WM_RBUTTONUP:
            p->mouseDown[1] = false; p->mouseReleased[1] = true;
            return 0;
        case WM_MBUTTONDOWN:
            p->mouseDown[2] = true; p->mousePressed[2] = true;
            return 0;
        case WM_MBUTTONUP:
            p->mouseDown[2] = false; p->mouseReleased[2] = true;
            return 0;
        case WM_MOUSEWHEEL:
            p->wheel += (float)GET_WHEEL_DELTA_WPARAM(wp) / (float)WHEEL_DELTA;
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wp < 256) {
                if (!p->keys[wp]) p->keyPressed[wp] = true;
                p->keys[wp] = true;
            }
            if (wp == VK_F4 && (GetKeyState(VK_MENU) & 0x8000)) { p->running = false; return 0; }
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wp < 256) p->keys[wp] = false;
            return 0;
        case WM_KILLFOCUS:
            for (int i = 0; i < 256; ++i) p->keys[i] = false;
            for (int i = 0; i < 3; ++i) p->mouseDown[i] = false;
            return 0;
        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool Platform::init(HINSTANCE instance, int width, int height, const wchar_t* title) {
    inst = instance;
    w = width; h = height;
    g_platform = this;

    SetProcessDPIAware();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"SolarisHistoryWindow";
    if (!RegisterClassExW(&wc)) return false;

    RECT rc = {0, 0, width, height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    int ww = rc.right - rc.left, wh = rc.bottom - rc.top;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    hwnd = CreateWindowExW(0, wc.lpszClassName, title, WS_OVERLAPPEDWINDOW,
                           sx, sy, ww, wh, nullptr, nullptr, instance, nullptr);
    if (!hwnd) return false;

    hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf) return false;
    SetPixelFormat(hdc, pf, &pfd);
    glrc = wglCreateContext(hdc);
    if (!glrc) return false;
    wglMakeCurrent(hdc, glrc);

    // vsync si esta disponible
    typedef BOOL(WINAPI * PFNSwapInterval)(int);
    PFNSwapInterval swapInterval = (PFNSwapInterval)wglGetProcAddress("wglSwapIntervalEXT");
    if (swapInterval) swapInterval(1);

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    startTime = now();
    return true;
}

void Platform::pump() {
    for (int i = 0; i < 3; ++i) { mousePressed[i] = false; mouseReleased[i] = false; }
    for (int i = 0; i < 256; ++i) keyPressed[i] = false;
    wheel = 0;
    pmx = mx; pmy = my;
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) running = false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (!IsWindowVisible(hwnd)) running = false;
    if (w < 100 || h < 100) { RECT rc; GetClientRect(hwnd, &rc); w = rc.right; h = rc.bottom; }
    setCursorHand(false);
}

void Platform::swap() { SwapBuffers(hdc); }

void Platform::shutdown() {
    if (glrc) { wglMakeCurrent(nullptr, nullptr); wglDeleteContext(glrc); glrc = nullptr; }
    if (hdc) { ReleaseDC(hwnd, hdc); hdc = nullptr; }
    if (hwnd) { DestroyWindow(hwnd); hwnd = nullptr; }
}

double Platform::now() const {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
}

void Platform::setCursorHand(bool hand) {
    static bool current = false;
    if (hand == current) return;
    current = hand;
    SetCursor(LoadCursor(nullptr, hand ? IDC_HAND : IDC_ARROW));
}

} // namespace sol
