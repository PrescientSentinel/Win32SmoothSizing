#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>

#include <windows.h>

#include "glad/glad.h"

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

typedef BOOL(WINAPI *wglSwapIntervalEXT_t)(int interval);
static wglSwapIntervalEXT_t wglSwapInterval = nullptr;

static HGLRC gl_render_context;
static unsigned int shaderProgram;
static unsigned int VAO;

static float vertices[] = {
     0.5f,  0.5f, 0.0f,  // top right
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f,  // bottom left
    -0.5f,  0.5f, 0.0f   // top left
};

unsigned int indices[] = {  // note that we start from 0!
    0, 1, 3,  // first Triangle
    1, 2, 3   // second Triangle
};

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

        // build and compile our shader program
        // ------------------------------------
        // vertex shader
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // check for shader compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s\n", infoLog);
        }
        // fragment shader
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // check for shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
        }
        // link shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // check for linking errors
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // set up vertex data (and buffer(s)) and configure vertex attributes
        // ------------------------------------------------------------------
        unsigned int VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);

        // uncomment this call to draw in wireframe polygons.
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        return 0;
    }

    case WM_DESTROY: {
        // HDC hdc = GetDC(hwnd);
        // wglMakeCurrent(hdc, NULL);
        // wglDeleteContext(gl_render_context);
        PostQuitMessage(0);
        return 0;
    }

    case WM_PAINT: {
        BeginPaint(hwnd, NULL);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw our first triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time

        HDC hdc = GetDC(hwnd);
        SwapBuffers(hdc);
        ReleaseDC(hwnd, hdc);

        EndPaint(hwnd, NULL);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
