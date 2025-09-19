#ifndef UNICODE
#define UNICODE
#endif

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <windows.h>

#include "glad/glad.h"

const char *vertex_shader_source =
    "#version 330 core\n"

    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"

    "out vec3 color;\n"

    "uniform float modifier;\n"

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

// Used for timing
static int64_t perf_freq;
static int64_t initial_perf_count;

const float vertices[] = {
     0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // top right
     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom left
    -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // top left
};

const unsigned int indices[] = {
    0, 1, 2, // first triangle
    0, 2, 3, // second triangle
};

const float clear_color[] = { 0.1f, 0.1f, 0.1f, 1.0f };

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
    double result = (double)(end_count - start_count) / perf_freq;
    return result;
}

double get_time_now() {
    int64_t count_now = get_perf_count();
    double duration_since_init = time_duration_seconds(initial_perf_count, count_now);
    return duration_since_init;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    // SetProcessDPIAware();
    timer_init();

    // --------------------------------------------------
    // ----- Create the window
    // --------------------------------------------------
    // Register the window class
    WNDCLASSEX wind_class = {};
    wind_class.cbSize = sizeof(WNDCLASSEX);
    wind_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // The first two flags together say: "WM_SIZE should trigger WM_PAINT"
    wind_class.lpfnWndProc = WindowProc;
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
        L"RenderThread",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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

    HGLRC gl_render_context = wglCreateContext(hdc);
    wglMakeCurrent(hdc, gl_render_context);

    if (!gladLoadGL()) {
        OutputDebugString(L"Could not load OpenGL functions\n");
        return 0;
    }

    // MessageBoxA(0,(char*)glGetString(GL_VERSION), "OPENGL VERSION", 0);

    typedef BOOL(WINAPI *wglSwapIntervalEXT_t)(int interval);
    wglSwapIntervalEXT_t wglSwapInterval = (wglSwapIntervalEXT_t)wglGetProcAddress("wglSwapIntervalEXT");
    if (!wglSwapInterval) {
        OutputDebugString(L"Could not load wglSwapInterval function. Cannot enable V-Sync\n");
        // TODO: If we cannot enable V-Sync and it is not enabled on a system level, we should
        // probably sleep to not consume CPU needlessly
    }
    else {
        wglSwapInterval(1); // Enable V-Sync
    }

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
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        sprintf_s(err_buf, sizeof(err_buf), "Vertex shader compilation failed\n%s\n", info_log);
        OutputDebugStringA(err_buf);
    }

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        sprintf_s(err_buf, sizeof(err_buf), "Fragment shader compilation failed\n%s\n", info_log);
        OutputDebugStringA(err_buf);
    }

    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
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

    // --------------------------------------------------
    // ----- Show window and enter main loop
    // --------------------------------------------------
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    bool should_quit = false;
    while (!should_quit) {
        int64_t frame_begin = get_perf_count();

        // Drain the message queue first
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
            case WM_QUIT:
                should_quit = true;
            case WM_KEYDOWN:
                if (msg.wParam == VK_ESCAPE)
                    PostQuitMessage(0);
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Set uniform for animation
        float time = (float)get_time_now();
        glUseProgram(shader_program);
        GLint modifier_uniform = glGetUniformLocation(shader_program, "modifier");
        glUniform1f(modifier_uniform, 0.25f * sinf(2.0f * time) + 0.5f);

        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        SwapBuffers(hdc);

        // Insert frame time into window title
        int64_t frame_end = get_perf_count();
        float frame_time = (float)time_duration_seconds(frame_begin, frame_end);
        wchar_t buf[64];
        swprintf(buf, sizeof(buf), L"RenderThread - frame time: %fs", frame_time);
        SetWindowText(hwnd, buf);
    }

    wglDeleteContext(gl_render_context);

    return 0;
}

// TODO: All code interfacing with OpenGL will need to move to seperate thread
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }

    case WM_PAINT: {
        BeginPaint(hwnd, NULL);
        EndPaint(hwnd, NULL);
        return 0;
    }

    case WM_SIZE: {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        glViewport(0, 0, width, height);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
