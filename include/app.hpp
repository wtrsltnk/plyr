#ifndef APP_H
#define APP_H

#include <chrono>
#include <filesystem>
#include <glm/glm.hpp>
#include <glprogram.hpp>
#include <string>
#include <vector>
#include <vertexarray.hpp>

#include <imgui.h>

#include <IconsFontaudio.h>
#include <IconsLucide.h>

enum ePlaylistMode
{
    Playlist,
    FindFile,
};

class App
{
public:
    App(const std::vector<std::string> &args);
    virtual ~App();

    bool Init();
    int Run();
    void Quit();

    void OnInit();
    void OnFrame(std::chrono::nanoseconds diff);
    void OnResize(int width, int height);
    void OnExit();

    template <class T>
    T *GetWindowHandle() const;

    static void *_render;
    static std::vector<std::filesystem::path> _playlist;
    static int _current_playing_index;

protected:
    const std::vector<std::string> &_args;
    glm::mat4 _projection;
    int _width = 0;
    int _height = 0;
    int _requestedHeight = 0;
    bool _isCollapsed = false;
    const int collapsedHeight = 180;
    const int expandedHeight = 768;

    template <class T>
    void SetWindowHandle(T *handle);

    void ClearWindowHandle();
    ImFont *fad_icon_font = nullptr;
    ImFont *header_font = nullptr;

    float headerOffset = 0;
    void PlayPlaylistItem(int index);
    static void OnSongEnded(void *userdata);

    void RenderFrame();

private:
    int playState = 0;
    std::string _currentPlaying;
    int _selected = 0;
    float progress = 0.0f;
    ePlaylistMode playlistMode = ePlaylistMode::Playlist;
    std::filesystem::path findFileStartDir;
    std::filesystem::path _fileRoot;
    int selectedFile = 0;
    std::vector<std::filesystem::path> foldersInCurrentDir;
    std::vector<std::filesystem::path> filesInCurrentDir;

    void DrawTitleTicker();
    void DrawPlaybackControls();
    void DrawClock();
    void DrawSpectrum();
    void DrawTimeline();
    void DrawPlaylist();
    void DrawFileSelector();

    void OpenSelectedFile();
    void ListFoldersAndFiles();

    void SetWindowHeight(int height);

private:
    void *_windowHandle;
};

#endif // APP_H
