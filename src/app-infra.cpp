#include <app.hpp>

// Make sure GLAD is included before SDL2
#include <glad/glad.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "audio_sdl.h"

#define OPENGL_LATEST_VERSION_MAJOR 4
#define OPENGL_LATEST_VERSION_MINOR 6

static char szProgramName[] = "plyr";

// SDL3 hit test callback for custom title bar dragging
static SDL_HitTestResult SDLCALL HitTestCallback(SDL_Window *window, const SDL_Point *pt, void *data)
{
    const int titleBarHeight = 40;   // Approximate ImGui title bar height
    const int closeButtonWidth = 50; // Reserve space for close button on the right

    // Get window size
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    // Top area is draggable (title bar), but exclude close button area
    if (pt->y < titleBarHeight)
    {
        // Don't make the close button area draggable (top-right corner)
        if (pt->x > w - closeButtonWidth)
        {
            return SDL_HITTEST_NORMAL; // Allow clicks on close button
        }

        return SDL_HITTEST_DRAGGABLE;
    }

    return SDL_HITTEST_NORMAL;
}

void *App::_render = nullptr;
std::vector<std::filesystem::path> App::_playlist;
int App::_current_playing_index = -1;

struct WindowHandle
{
    SDL_Window *window;
};

App::App(const std::vector<std::string> &args)
    : _args(args)
{
    // Initialize file root from command-line args or use default
    if (args.size() > 1)
    {
        std::filesystem::path candidate(args[1]);
        if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate))
        {
            _fileRoot = std::filesystem::canonical(candidate);
            std::cout << "Using music folder: " << _fileRoot.string() << std::endl;
        }
        else
        {
            std::cerr << "Warning: '" << args[1] << "' is not a valid directory, using default" << std::endl;
            _fileRoot = std::filesystem::current_path();
        }
    }
    else
    {
        _fileRoot = std::filesystem::current_path();
    }
}

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

void APIENTRY OpenGLMessageCallback(
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
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return false;
    }

    auto window = SDL_CreateWindow(
        szProgramName,
        1024,
        collapsedHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (window == 0)
    {
        std::cout << "Failed to create SDL3 window" << std::endl;

        SDL_Quit();

        return false;
    }

    SDL_SetWindowProgressState(window, SDL_ProgressState::SDL_PROGRESS_STATE_PAUSED);

    // SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_LATEST_VERSION_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_LATEST_VERSION_MINOR);

    auto context = SDL_GL_CreateContext(window);
    if (context == NULL)
    {
        std::cout << "Failed to create SDL3 GL context" << std::endl;

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

    // Enable custom hit testing for borderless window dragging
    SDL_SetWindowHitTest(window, HitTestCallback, nullptr);

    SDL_SetWindowMinimumSize(window, 0, collapsedHeight);

    float baseFontSize = 20.0f;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // const char *fontName = "Doto-Bold.ttf";
    const char *fontName = "UbuntuSansMono-Regular.ttf";
    io.Fonts->AddFontFromFileTTF(fontName, baseFontSize, NULL, io.Fonts->GetGlyphRangesDefault());

    // 13.0f is the size of the default font. Change to the font size you use.
    float iconFontSize = baseFontSize;

    {
        static const ImWchar icons_ranges[] = {ICON_MIN_FAD, ICON_MAX_16_FAD, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphOffset.y = 5;
        // icons_config.GlyphMinAdvanceX = iconFontSize;
        //    fad_icon_font = io.Fonts->AddFontFromFileTTF("thirdparty/IconFontCppHeaders/lucide.ttf", iconFontSize, &icons_config, icons_ranges);
        fad_icon_font = io.Fonts->AddFontFromFileTTF("thirdparty/IconFontCppHeaders/fontaudio.ttf", iconFontSize, &icons_config, icons_ranges);
    }

    {
        static const ImWchar icons_ranges[] = {ICON_MIN_LC, ICON_MAX_16_LC, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphOffset.y = 5;
        // icons_config.GlyphMinAdvanceX = iconFontSize;
        //    fad_icon_font = io.Fonts->AddFontFromFileTTF("thirdparty/IconFontCppHeaders/lucide.ttf", iconFontSize, &icons_config, icons_ranges);
        fad_icon_font = io.Fonts->AddFontFromFileTTF("thirdparty/IconFontCppHeaders/lucide.ttf", iconFontSize, &icons_config, icons_ranges);
    }

    ImFontConfig header_config;
    header_config.MergeMode = false;
    header_config.PixelSnapH = true;
    header_config.GlyphOffset.y = 5;
    // icons_config.GlyphMinAdvanceX = iconFontSize;
    //    fad_icon_font = io.Fonts->AddFontFromFileTTF("thirdparty/IconFontCppHeaders/lucide.ttf", iconFontSize, &icons_config, icons_ranges);
    header_font = io.Fonts->AddFontFromFileTTF(
        fontName,
        iconFontSize + 8,
        &header_config,
        io.Fonts->GetGlyphRangesDefault());

    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    auto &style = ImGui::GetStyle();
    style.FrameBorderSize = 0;
    style.FramePadding = ImVec2(12.0f, 10.0f);
    style.FrameRounding = 0.0f;
    style.WindowBorderSize = 0;

    // ImGui::StyleColorsClassic();
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Border] = ImVec4(0.26f, 0.26f, 0.26f, 0.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(15.0f / 255.0f, 27.0f / 255.0f, 67.0f / 255.0f, 0.6f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(76.0f / 255.0f, 35.0f / 255.0f, 113.0f / 255.0f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(34.0f / 255.0f, 29.0f / 255.0f, 95.0f / 255.0f, 0.3f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(88.0f / 255.0f, 45.0f / 255.0f, 122.0f / 255.0f, 0.3f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(56.0f / 255.0f, 206.0f / 255.0f, 248.0f / 255.0f, 0.3f);
    style.Colors[ImGuiCol_Header] = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_HeaderActive] = style.Colors[ImGuiCol_ButtonActive];
    style.Colors[ImGuiCol_HeaderHovered] = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_TitleBg] = ImVec4(33.0f / 255.0f, 34.0f / 255.0f, 91.0f / 255.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(33.0f / 255.0f, 34.0f / 255.0f, 91.0f / 255.0f, 1.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 150");

    if (GLVersion.major >= 3)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        // glDebugMessageCallback(OpenGLMessageCallback, nullptr);

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

void App::SetWindowHeight(int height)
{
    _requestedHeight = height;
}

int App::Run()
{
    bool running = true;

    auto windowHandle = std::unique_ptr<WindowHandle>(GetWindowHandle<WindowHandle>());

    std::thread t1([&running] {
        while (running)
        {
            audio_pump(&App::_render);

            SDL_Delay(10);
        }
    });

    int cachedW = 0, cachedH = 0;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                auto width = (event.window.data1 <= 0 ? 1 : event.window.data1);
                auto height = (event.window.data2 <= 0 ? 1 : event.window.data2);

                printf("GameLoop w=%d, h=%d\n", width, height);
                fflush(stdout);
                cachedW = width;
                cachedH = height;
                OnResize(width, height);

                RenderFrame();
            }
            else if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }

            ImGui_ImplSDL3_ProcessEvent(&event);
        }

        if (playState == 1)
        {
            SDL_SetWindowProgressState(windowHandle->window, SDL_ProgressState::SDL_PROGRESS_STATE_NORMAL);
            SDL_SetWindowProgressValue(windowHandle->window, progress);
        }
        else if (playState == 2)
        {
            SDL_SetWindowProgressState(windowHandle->window, SDL_ProgressState::SDL_PROGRESS_STATE_PAUSED);
        }
        else
        {
            SDL_SetWindowProgressState(windowHandle->window, SDL_ProgressState::SDL_PROGRESS_STATE_NONE);
        }

        int w, h;
        SDL_GetWindowSize(windowHandle->window, &w, &h);
        if (w != cachedW || h != cachedH)
        {
            cachedW = w;
            cachedH = h;
            OnResize(w, h);
        }

        SDL_Delay(10);

        glClearColor(0, 0, 0, 0);

        RenderFrame();

        if (_requestedHeight > 0)
        {
            SDL_SetWindowResizable(windowHandle->window, _requestedHeight != collapsedHeight);

            int h, w;
            SDL_GetWindowSize(windowHandle->window, &w, &h);

            SDL_SetWindowSize(windowHandle->window, w, _requestedHeight);
            OnResize(w, _requestedHeight);
            _requestedHeight = 0;

            RenderFrame();
        }

        SDL_GL_SwapWindow(windowHandle->window);
    }

    running = false;

    t1.join();

    ClearWindowHandle();

    //  SDL_Quit();

    return 0;
}

void App::RenderFrame()
{
    static auto prev = std::chrono::steady_clock::now();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    auto now = std::chrono::steady_clock::now();
    OnFrame(std::chrono::duration_cast<std::chrono::nanoseconds>(now - prev));
    prev = now;

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::Quit()
{
    SDL_Event ev;
    ev.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&ev);
}
