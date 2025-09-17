#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>

#include <windows.h>

#include "glad/glad.h"

typedef BOOL(WINAPI *wglSwapIntervalEXT_t)(int interval);
static wglSwapIntervalEXT_t wglSwapInterval = nullptr;

static HGLRC gl_render_context;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    // SetProcessDPIAware();
    // timeBeginPeriod(1);

    // Register the window class
    WNDCLASSEX wind_class = {};
    wind_class.cbSize = sizeof(WNDCLASSEX);
    wind_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // The first two flags together say: "WM_SIZE should trigger WM_PAINT"
    wind_class.lpfnWndProc = WindowProc;
    wind_class.hInstance = hInstance;
    wind_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    wind_class.hbrBackground = NULL; // We replace the window's entire contents every frame ourselves
    wind_class.lpszClassName = L"ResponsiveRenderingWindow";

    ATOM wnd_class_atom = RegisterClassEx(&wind_class);
    if (!wnd_class_atom) {
        OutputDebugString(L"Could not register Window Class");
        return 1;
    }

    HWND hwnd = CreateWindowEx(
        0,
        wind_class.lpszClassName,
        L"RenderThread",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL) {
        OutputDebugString(L"Could not create Window");
        return 1;
    }
    OutputDebugString(L"Window created successfully\n");

    // UpdateWindow(hWnd);
    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    // GetMessage returns 0 when it retrieves a WM_QUIT message
    while (GetMessage(&msg, NULL, 0, 0)) {
        switch (msg.message) {
        case WM_KEYDOWN:
            if (msg.wParam == 'Q')
                PostQuitMessage(0);
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// TODO: All code interfacing with OpenGL will need to move to seperate thread
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
            PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
            32,                   // Colordepth of the framebuffer.
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            24,                   // Number of bits for the depthbuffer
            8,                    // Number of bits for the stencilbuffer
            0,                    // Number of Aux buffers in the framebuffer.
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };

        HDC hdc = GetDC(hwnd);
        int pf = ChoosePixelFormat(hdc, &pfd);
        if (!pf) {
            OutputDebugString(L"Could not find pixel format\n");
            PostQuitMessage(1);
            return 0;
        }
        else {
            SetPixelFormat(hdc, pf, &pfd);
        }

        gl_render_context = wglCreateContext(hdc);
        wglMakeCurrent(hdc, gl_render_context);

        if (!gladLoadGL()) {
            OutputDebugString(L"Could not load OpenGL functions");
            PostQuitMessage(1);
            return 0;
        }

        // MessageBoxA(0,(char*)glGetString(GL_VERSION), "OPENGL VERSION", 0);

        // wglSwapInterval = (wglSwapIntervalEXT_t)wglGetProcAddress("wglSwapIntervalEXT");
        // wglSwapInterval(1); // Enable V-Sync

        // glViewport(0, 0, 800, 600);

        return 0;
    }

    case WM_DESTROY: {
        HDC hdc = GetDC(hwnd);
        wglMakeCurrent(hdc, NULL);
        wglDeleteContext(gl_render_context);
        PostQuitMessage(0);
        return 0;
    }

    case WM_PAINT: {
        BeginPaint(hwnd, NULL);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        HDC hdc = GetDC(hwnd);
        SwapBuffers(hdc);
        // ReleaseDC(hwnd, hdc);

        EndPaint(hwnd, NULL);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
