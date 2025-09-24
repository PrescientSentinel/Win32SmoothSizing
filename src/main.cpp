#ifndef UNICODE
#define UNICODE
#endif

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <windows.h>

#include "glad/glad.h"
#include "glad/glad_wgl.h"

// --------------------------------------------------
// ----- DATA
const char *vertex_shader_source =
    "#version 330 core\n"

    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"

    "out vec3 color;\n"

    "uniform float modifier = 1.0;\n"

    "void main()\n"
    "{\n"
    "    gl_Position = vec4(aPos.x * modifier, aPos.y * modifier, aPos.z, 1.0);\n"
    "    color = aColor;\n"
    "}\0";

const char *fragment_shader_source =
    "#version 330 core\n"

    "in vec3 color;\n"
    "out vec4 fragColor;\n"

    "void main()\n"
    "{\n"
    "    fragColor = vec4(color, 1.0f);\n"
    "}\n\0";

const float vertices[] = {                // (x, y, z, r, g, b)
     0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // top right
     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom left
    -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // top left
};

const unsigned int indices[] = {
    0, 1, 2, // first triangle
    0, 2, 3, // second triangle
};

// --------------------------------------------------
// ----- CONSTANTS
const float pi = 3.14159265358979f;
const float background_color[] = { 0.1f, 0.1f, 0.1f, 1.0f };
const int window_width = 800;
const int window_height = 600;

// --------------------------------------------------
// ----- GLOBALS
// Used for timing
static int64_t perf_freq;
static int64_t initial_perf_count;

static HGLRC gl_render_context;
static unsigned int shader_program;

// --------------------------------------------------
// ----- HELPERS
int64_t get_perf_count() {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (int64_t)count.QuadPart;
}

void timer_init() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    perf_freq =  freq.QuadPart;
    initial_perf_count = get_perf_count();
}

double time_duration_seconds(int64_t start_count, int64_t end_count) {
    return (double)(end_count - start_count) / perf_freq;
}

double get_time_now() {
    int64_t count_now = get_perf_count();
    return time_duration_seconds(initial_perf_count, count_now);
}

bool is_key_repeating(LPARAM lParam) {
    return (lParam & (1 << 30)) >> 30;
}
// --------------------------------------------------

struct WindowData {
    HWND hwnd;
    CRITICAL_SECTION crit_sect;
    CONDITION_VARIABLE cond_var;
    int width;
    int height;
    int new_width;
    int new_height;
    bool size_changed;
    bool terminate;
    bool animate;
};

void render_thread_func(WindowData *window) {
    GLsync fence;

    HDC hdc = GetDC(window->hwnd);
    wglMakeCurrent(hdc, gl_render_context);

    float time = (float)get_time_now();
    float start_time = time;

    // While the main thread hasn't signaled to stop
    while (true) {
        EnterCriticalSection(&window->crit_sect);

        float sleep_time = 0;
        if (!window->animate && !window->size_changed) {
            float before_sleep_time = (float)get_time_now();
            SleepConditionVariableCS(&window->cond_var, &window->crit_sect, INFINITE);
            float end_sleep_time = (float)get_time_now();
            sleep_time = end_sleep_time - before_sleep_time;
        }

        bool terminate = window->terminate;
        bool animate = window->animate;
        bool size_changed = window->size_changed;
        window->size_changed = false;

        LeaveCriticalSection(&window->crit_sect);

        if (terminate) break;

        if (size_changed) {
            RECT rect;
            GetClientRect(window->hwnd, &rect);
            glViewport(0, 0, rect.right, rect.bottom);
        }

        glUseProgram(shader_program);

        if (animate) {
            GLint modifier_uniform = glGetUniformLocation(shader_program, "modifier");
            glUniform1f(modifier_uniform, 0.25f * sinf(2.0f * (time + pi / 4)) + 0.75f);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        SwapBuffers(hdc);

        fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        if (fence) {
            glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1'000'000'000);
            glDeleteSync(fence);
        }

        float end_time = (float)get_time_now();
        time += end_time - start_time - sleep_time;
        start_time = end_time;

        WakeConditionVariable(&window->cond_var);
    }

    ReleaseDC(window->hwnd, hdc);
    OutputDebugStringA("RenderThread exiting\n");

    WakeConditionVariable(&window->cond_var);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WindowData *window = (WindowData*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_CLOSE: {
        EnterCriticalSection(&window->crit_sect);
        window->terminate = true;
        WakeConditionVariable(&window->cond_var);
        LeaveCriticalSection(&window->crit_sect);

        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY: {DeleteCriticalSection(&window->crit_sect);
        PostQuitMessage(0);
        return 0;
    }

    case WM_PAINT: {
        BeginPaint(hwnd, NULL);

        EnterCriticalSection(&window->crit_sect);

        if ((window->width != window->new_width) | (window->height != window->new_height)) {
            window->new_width = window->width;
            window->new_height = window->height;
            window->size_changed = true;
        }

        WakeConditionVariable(&window->cond_var);
        SleepConditionVariableCS(&window->cond_var, &window->crit_sect, INFINITE);
        LeaveCriticalSection(&window->crit_sect);

        EndPaint(hwnd, NULL);
        return 0;
    }

    case WM_SIZE: {
        window->width = LOWORD(lParam);
        window->height = HIWORD(lParam);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    SetProcessDPIAware();
    timer_init();

    // --------------------------------------------------
    // ----- Create the window
    // --------------------------------------------------
    // Register the window class
    WNDCLASSEX wind_class = {};
    wind_class.cbSize = sizeof(WNDCLASSEX);
    wind_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // The first two flags together say: "WM_SIZE should trigger WM_PAINT"
    wind_class.lpfnWndProc = DefWindowProc; // Use default proc until everything is set up
    wind_class.hInstance = hInstance;
    wind_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    wind_class.hbrBackground = NULL; // We replace the window's entire contents every frame ourselves
    wind_class.lpszClassName = L"ResponsiveRenderingWindow";

    if (!RegisterClassEx(&wind_class)) {
        OutputDebugString(L"Could not register Window Class\n");
        return 1;
    }

    HWND hwnd = CreateWindowEx(
        0,
        wind_class.lpszClassName,
        L"Press Space to pause/resume animation",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_width, window_height,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL) {
        OutputDebugString(L"Could not create Window\n");
        return 1;
    }

    // --------------------------------------------------
    // ----- Set up OpenGL
    // --------------------------------------------------
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    HDC hdc = GetDC(hwnd);

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf) {
        OutputDebugString(L"Could not find pixel format\n");
        return 1;
    }
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC dummy_context = wglCreateContext(hdc);
    wglMakeCurrent(hdc, dummy_context);

    if (!gladLoadGL()) {
        OutputDebugString(L"Could not load OpenGL functions\n");
        return 1;
    }

    if (!gladLoadWGL(hdc)) {
        OutputDebugString(L"Could not load OpenGL functions\n");
        return 1;
    }

    int pf_attribs[] = {
      WGL_DRAW_TO_WINDOW_ARB, 1,
      WGL_SUPPORT_OPENGL_ARB, 1,
      WGL_DOUBLE_BUFFER_ARB, 1,
      WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
      WGL_COLOR_BITS_ARB, 32,
      WGL_DEPTH_BITS_ARB, 24,
      WGL_STENCIL_BITS_ARB, 8,
      0
    };

    UINT num_formats = 0;
    pf = 0;
    // Just get one pixel format
    wglChoosePixelFormatARB(hdc, pf_attribs, 0, 1, &pf, &num_formats);

    int context_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        0,
    };

    // Set global real context
    gl_render_context = wglCreateContextAttribsARB(hdc, dummy_context, NULL);
    wglMakeCurrent(hdc, 0);
    wglDeleteContext(dummy_context);
    wglMakeCurrent(hdc, gl_render_context);

    // MessageBoxA(0, (char*)glGetString(GL_VERSION), "OPENGL VERSION", 0);

    wglSwapIntervalEXT(1); // V-Sync
    // TODO: If not enabling V-Sync, we should probably sleep the render thread to not consume CPU

    // --------------------------------------------------
    // ----- Compile shaders and create shader program
    // --------------------------------------------------
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    int success;
    char info_log[512];
    char err_buf[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, sizeof(info_log), NULL, info_log);
        sprintf_s(err_buf, sizeof(err_buf), "Vertex shader compilation failed\n%s\n", info_log);
        OutputDebugStringA(err_buf);
    }

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, sizeof(info_log), NULL, info_log);
        sprintf_s(err_buf, sizeof(err_buf), "Fragment shader compilation failed\n%s\n", info_log);
        OutputDebugStringA(err_buf);
    }

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, sizeof(info_log), NULL, info_log);
        sprintf_s(err_buf, sizeof(err_buf), "Shader program linking failed\n%s\n", info_log);
        OutputDebugStringA(err_buf);
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // --------------------------------------------------
    // ----- Set up vertex data and attributes
    // --------------------------------------------------
    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Colours
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Draw wireframe polygons

    glClearColor(background_color[0], background_color[1], background_color[2], background_color[3]);

    // --------------------------------------------------
    // ----- Prepare and create render thread
    // --------------------------------------------------
    wglMakeCurrent(hdc, NULL);

    WindowData *window = new WindowData; // TODO: memory 'leak'
    window->hwnd = hwnd;
    InitializeCriticalSection(&window->crit_sect);
    InitializeConditionVariable(&window->cond_var);
    window->size_changed = false;
    window->terminate = false;
    window->animate = false;

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window); // Attach data to window
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc); // Attach window procedure

    HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)render_thread_func, window, 0, NULL);

    UpdateWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);

    // --------------------------------------------------
    // ----- Enter main program loop
    // --------------------------------------------------
    bool should_quit = false;
    while (!should_quit) {
        // Drain the message queue first
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
            case WM_QUIT:
                should_quit = true;
            case WM_KEYDOWN:
                if (msg.wParam == VK_ESCAPE)
                    PostMessage(hwnd, WM_CLOSE, NULL, NULL);
                else if (msg.wParam == VK_SPACE && !is_key_repeating(msg.lParam)) {
                    EnterCriticalSection(&window->crit_sect);
                    window->animate = !window->animate;
                    WakeConditionVariable(&window->cond_var);
                    LeaveCriticalSection(&window->crit_sect);
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!should_quit)
            WaitMessage();
    }

    // Stop and wait on render thread before exiting
    WaitForSingleObject(thread, INFINITE);

    // Clean up, if necessary
    wglDeleteContext(gl_render_context);

    return 0;
}
