#include <app.hpp>

// Make sure GLAD is included before SDL2
#include <glad/glad.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>
#include <chrono>
#include <iostream>
#include <memory>

#define OPENGL_LATEST_VERSION_MAJOR 4
#define OPENGL_LATEST_VERSION_MINOR 6

static char szProgramName[] = "SDL2.EnTT.Bullet";

struct WindowHandle
{
    SDL_Window *window;
};

App::App(const std::vector<std::string> &args)
    : _args(args)
{}

App::~App() = default;

template <class T>
T *App::GetWindowHandle() const
{
    return reinterpret_cast<T *>(_windowHandle);
}

template <class T>
void App::SetWindowHandle(T *handle)
{
    _windowHandle = (void *)handle;
}

void App::ClearWindowHandle()
{
    _windowHandle = nullptr;
}

void OpenGLMessageCallback(
    unsigned source,
    unsigned type,
    unsigned id,
    unsigned severity,
    int length,
    const char *message,
    const void *userParam)
{
    (void)userParam;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            std::cout << "CRITICAL";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cout << "ERROR";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            std::cout << "WARNING";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cout << "DEBUG";
            break;
        default:
            std::cout << "UNKNOWN";
            break;
    }

    std::cout << "\n    source    : " << source
              << "\n    message   : " << message
              << "\n    type      : " << type
              << "\n    id        : " << id
              << "\n    length    : " << length
              << "\n";
}

bool App::Init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return false;
    }

    auto window = SDL_CreateWindow(szProgramName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_OPENGL);
    if (window == 0)
    {
        std::cout << "Failed to create SDL2 window" << std::endl;

        SDL_Quit();

        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_LATEST_VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_LATEST_VERSION_MINOR);

    auto context = SDL_GL_CreateContext(window);
    if (context == NULL)
    {
        std::cout << "Failed to create SDL2 GL context" << std::endl;

        SDL_Quit();

        return false;
    }

    SDL_GL_MakeCurrent(window, context);

    if (!gladLoadGL())
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;

        SDL_Quit();

        return false;
    }

    SetWindowHandle(new WindowHandle({
        window,
    }));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 150");

    if (GLVersion.major >= 3)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OpenGLMessageCallback, nullptr);

        glDebugMessageControl(
            GL_DONT_CARE,
            GL_DONT_CARE,
            GL_DEBUG_SEVERITY_NOTIFICATION,
            0,
            nullptr,
            GL_FALSE);
    }

    std::cout << "running opengl " << GLVersion.major << "." << GLVersion.minor << std::endl;

    OnInit();

    OnResize(1024, 768);

    return true;
}

int App::Run()
{
    bool running = true;

    auto windowHandle = std::unique_ptr<WindowHandle>(GetWindowHandle<WindowHandle>());

    auto prev = std::chrono::steady_clock::now();
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    auto width = (event.window.data1 <= 0 ? 1 : event.window.data1);
                    auto height = (event.window.data2 <= 0 ? 1 : event.window.data2);

                    OnResize(width, height);
                }
            }
            else if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(windowHandle->window);
        ImGui::NewFrame();

        auto now = std::chrono::steady_clock::now();
        OnFrame(std::chrono::duration_cast<std::chrono::nanoseconds>(now - prev));
        prev = now;

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(windowHandle->window);
    }

    ClearWindowHandle();

    SDL_Quit();

    return 0;
}
