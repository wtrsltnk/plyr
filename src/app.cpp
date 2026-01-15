#include <app.hpp>

#include <Base64.h>
#include <entities.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glprogram.hpp>
#include <glshader.hpp>

#include "audio_sdl.h"

#include "decode.h"

decoder _dec;

#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void *data, size_t data_size, GLuint *out_texture, int *out_width, int *out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char *image_data = stbi_load_from_memory((const unsigned char *)data, (int)data_size, &image_width, &image_height, NULL, 4);

    if (image_data == NULL) return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
ImTextureID LoadTextureFromFileData(const std::string_view file_data)
{
    int my_image_width = 0;
    int my_image_height = 0;
    GLuint my_image_texture = 0;

    std::string s1{file_data.data(), file_data.size()};
    auto data = macaron::Base64::DecodeToBytes(s1);
    bool ret = LoadTextureFromMemory(data.data(), data.size(), &my_image_texture, &my_image_width, &my_image_height);

    IM_ASSERT(ret);

    return (ImTextureID)(intptr_t)my_image_texture;
}

ImTextureRef pauseImage, playImage, squareImage, skipForwardImage, skipBackImage, listMusicImage, settingsImage;
ImTextureRef folderImage, searchImage, trashImage, floppyImage, copyPlusImage, shuffleImage, arrowUpImage, arrowDownImage;

void App::OnInit()
{
    glClearColor(0.56f, 0.7f, 0.67f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // Set up auto-play callback with this App instance
    sdl_audio_set_end_callback(_render, &App::OnSongEnded, this);

    pauseImage = LoadTextureFromFileData(pauseImageData);
    playImage = LoadTextureFromFileData(playImageData);
    squareImage = LoadTextureFromFileData(squareImageData);
    skipForwardImage = LoadTextureFromFileData(skipForwardImageData);
    skipBackImage = LoadTextureFromFileData(skipBackImageData);
    listMusicImage = LoadTextureFromFileData(listMusicImageData);
    settingsImage = LoadTextureFromFileData(settingsImageData);
    folderImage = LoadTextureFromFileData(folderImageData);
    searchImage = LoadTextureFromFileData(searchImageData);
    trashImage = LoadTextureFromFileData(trashImageData);
    floppyImage = LoadTextureFromFileData(floppyImageData);
    copyPlusImage = LoadTextureFromFileData(copyPlusImageData);
    shuffleImage = LoadTextureFromFileData(shuffleImageData);
    arrowUpImage = LoadTextureFromFileData(arrowUpImageData);
    arrowDownImage = LoadTextureFromFileData(arrowDownImageData);
}

void App::OnResize(
    int width,
    int height)
{
    if (height < 1) height = 1;

    _width = width;
    _height = height;

    glViewport(0, 0, width, height);

    _projection = glm::perspective(glm::radians<float>(90), float(width) / float(height), 0.1f, 1000.0f);
}

void App::OnFrame(
    std::chrono::nanoseconds diff)
{
    // Scroll speed: pixels per second (50 pixels/sec)
    float scroll_speed = 50.0f;
    headerOffset += scroll_speed * (diff.count() / 1000000000.0f);

    // Safe progress calculation (avoid division by zero)
    if (_dec.mp3d.samples > 0)
    {
        progress = (float(_dec.mp3d.cur_sample) / float(_dec.mp3d.samples));
    }
    else
    {
        progress = 0.0f;
    }

    // Decay spectrum when paused or stopped
    if (playState == 0 || playState == 2)
    {
        decay_spectrum(&_dec);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
    {
        if (playState == 1)
        {
            sdl_audio_pause(_render, 1);
            playState = 2;
        }
        else if (playState == 2)
        {
            sdl_audio_pause(_render, 0);
            playState = 1;
        }
        else
        {
            PlayPlaylistItem(_selected);
        }
    }

    float seconds = 10.0f * (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) ? 6.0f : 1.0f);

    float stepSize = ((1.0f / _dec.mp3d.samples) * (_dec.mp3d.info.hz * _dec.mp3d.info.channels)) * seconds;

    if (playState == 1 && ImGui::IsKeyPressed(ImGuiKey_RightArrow, true))
    {
        progress += stepSize;
        mp3dec_ex_seek(&_dec.mp3d, uint64_t(progress * _dec.mp3d.samples));
    }

    if (playState == 1 && ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true))
    {
        progress -= stepSize;
        mp3dec_ex_seek(&_dec.mp3d, uint64_t(progress * _dec.mp3d.samples));
    }

    if (ImGui::IsKeyPressed(ImGuiKey_O, false) // ctrl+o
        && (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)))
    {
        playlistMode = ePlaylistMode::FindFile;
        ListFoldersAndFiles();
    }

    static bool is_open = true;

    // Always position ImGui window at (0,0) to fill the SDL window
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(_width, _height));
    ImGui::Begin("plyr", &is_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    DrawTitleTicker();

    DrawPlaybackControls();

    ImGui::SameLine();

    DrawSpectrum();

    ImGui::SameLine();

    DrawClock();

    DrawTimeline();

    if (_height > 400)
    {
        if (playlistMode == ePlaylistMode::Playlist)
        {
            DrawPlaylist();
        }
        else if (playlistMode == ePlaylistMode::FindFile)
        {
            DrawFileSelector();
        }
        else if (playlistMode == ePlaylistMode::Settings)
        {
            DrawSettings();
        }
    }

    ImGui::End();

    if (!is_open)
    {
        Quit();
    }

    // ImGui::ShowDemoWindow();
}

void App::DrawPlaybackControls()
{
    if (playState == 1)
    {
        if (ImGui::ImageButton("pause", pauseImage, ImVec2(24, 24)))
        {
            sdl_audio_pause(_render, 1);
            playState = 2;
        }
    }
    else if (playState == 2)
    {
        if (ImGui::ImageButton("play", playImage, ImVec2(24, 24)))
        {
            sdl_audio_pause(_render, 0);
            playState = 1;
        }
    }
    else
    {
        if (ImGui::ImageButton("play", playImage, ImVec2(24, 24)))
        {
            PlayPlaylistItem(_selected);
        }
    }
    ImGui::SameLine();

    if (ImGui::ImageButton("square", squareImage, ImVec2(24, 24)))
    {
        sdl_audio_set_dec(_render, 0);
        playState = 0;
        mp3dec_ex_seek(&_dec.mp3d, 0);
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("skip-back", skipBackImage, ImVec2(24, 24)))
    {
        _selected--;
        _selected = _selected % _playlist.size();

        PlayPlaylistItem(_selected);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Jump to the previous item of the playlist");
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("skip-forward", skipForwardImage, ImVec2(24, 24)))
    {
        _selected++;
        _selected = _selected % _playlist.size();

        PlayPlaylistItem(_selected);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Jump to the next item of the playlist");
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("list-music", listMusicImage, ImVec2(24, 24)) || (ImGui::IsKeyPressed(ImGuiKey_P) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)))
    {
        TogglePlaylist();
        if (collapsedHeight)
        {
            playlistMode = ePlaylistMode::Playlist;
        }
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Show/hide the playlist");
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("settings", settingsImage, ImVec2(24, 24)))
    {
        EnsurePlaylistVisible();
        playlistMode = ePlaylistMode::Settings;
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Show/hide settings");
    }
}

void App::DrawTitleTicker()
{
    ImGui::PushFont(header_font);

    std::string fn = _currentPlaying;
    if (fn.size() == 0)
    {
        fn = "No song playing";
    }

    // Remove extension if present
    size_t dot_pos = fn.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        fn = fn.substr(0, dot_pos);
    }

    auto cursorPosY = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(cursorPosY - 5);

    auto textSize = ImGui::CalcTextSize(fn.c_str());

    auto cursorPosX = ImGui::GetCursorPosX();
    if (textSize.x > ImGui::GetContentRegionAvail().x)
    {
        if (headerOffset > textSize.x)
        {
            headerOffset -= (textSize.x + 100);
        }
        ImGui::SetCursorPosX(cursorPosX - headerOffset);
    }

    ImGui::Text("%s", fn.c_str());

    if (textSize.x > ImGui::GetContentRegionAvail().x)
    {
        ImGui::SetCursorPosY(cursorPosY - 5);
        ImGui::SetCursorPosX((cursorPosX - headerOffset) + textSize.x + 100);

        ImGui::Text("%s", fn.c_str());
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
    ImGui::PopFont();
}

void App::DrawClock()
{
    char buf[256];

    // Check if we have valid decoder info
    if (_dec.mp3d.samples == 0 || _dec.mp3d.info.channels == 0 || _dec.mp3d.info.hz == 0)
    {
        sprintf_s(buf, 256, "00:00:00 / 00:00:00");
        ImGui::Text("%s", buf);
        return;
    }

    const int channelCount = _dec.mp3d.info.channels;
    const int sampleRate = _dec.mp3d.info.hz;

    auto currentSeconds = (_dec.mp3d.cur_sample / channelCount) / sampleRate;
    auto totalSeconds = (_dec.mp3d.samples / channelCount) / sampleRate;

    auto currentMinutes = int(std::floor(currentSeconds / 60.0));
    auto totalMinutes = int(std::floor(totalSeconds / 60.0));

    sprintf_s(buf, 256, "%02d:%02d:%02d / %02d:%02d:%02d",
              int(std::floor(currentMinutes / 60)),
              currentMinutes % 60,
              int(currentSeconds % 60),
              int(std::floor(totalMinutes / 60)),
              totalMinutes % 60,
              int(totalSeconds % 60));

    ImGui::Text("%s", buf);
}

void App::DrawSpectrum()
{
    const int num_bands = 32;
    const float bar_width = 5.0f;
    const float bar_spacing = 1.0f;
    const float max_height = 40.0f;
    const float scale = 0.4f; // Adjusted for new DFT-based spectrum

    ImVec2 cursor_pos = ImGui::GetCursorPos();
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();

    // Reserve space for the spectrum
    float total_width = num_bands * (bar_width + bar_spacing);
    ImGui::Dummy(ImVec2(total_width, max_height));

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    // Draw spectrum bars
    for (int band = 0; band < num_bands; band++)
    {
        // Average both channels
        float value = (_dec.spectrum[band][0] + _dec.spectrum[band][1]) * 0.5f;

        // Scale and clamp
        float height = (value * scale);
        if (height > max_height) height = max_height;
        if (height < 0) height = 0;

        float x = screen_pos.x + band * (bar_width + bar_spacing);
        float y_bottom = screen_pos.y + max_height;
        float y_top = y_bottom - height;

        // Color gradient from green to red based on height
        float intensity = height / max_height;
        ImU32 color = ImGui::GetColorU32(ImVec4(
            0.2f + intensity * 0.8f,
            0.8f - intensity * 0.6f,
            0.2f,
            1.0f));

        draw_list->AddRectFilled(
            ImVec2(x, y_top),
            ImVec2(x + bar_width, y_bottom),
            color);
    }
}

void App::DrawTimeline()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));    // background
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.0f, 0.7f, 0.0f, 1.0f)); // handle
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

    static ImVec2 lastMousePos = ImGui::GetMousePos();
    auto posX = ImGui::GetCursorPosX();
    auto avail = ImGui::GetContentRegionAvail();

    ImGui::Button("##slider", ImVec2(avail.x, 0));

    if (ImGui::IsItemActive() && (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDragging(ImGuiMouseButton_Left)))
    {
        auto mousePos = ImGui::GetMousePos();
        if (lastMousePos.x != mousePos.x)
        {
            progress = ((mousePos.x - posX) / avail.x);
            mp3dec_ex_seek(&_dec.mp3d, uint64_t(progress * _dec.mp3d.samples));
        }

        lastMousePos = mousePos;
    }

    // draw overlay fill manually
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    float filled_width = (p1.x - p0.x) * progress;

    ImU32 col = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
    ImGui::GetWindowDrawList()->AddRectFilled(p0, ImVec2(p0.x + filled_width, p1.y), col);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}

void App::ListFoldersAndFiles()
{
    if (findFileStartDir.empty())
    {
        findFileStartDir = _fileRoot;
    }

    foldersAndFilesInCurrentDir.clear();

    auto diritr = std::filesystem::directory_iterator(findFileStartDir);

    if (std::filesystem::canonical(findFileStartDir).compare(_fileRoot) > 0)
    {
        foldersAndFilesInCurrentDir.push_back(findFileStartDir / "..");
    }

    for (const auto &entry : diritr)
    {
        foldersAndFilesInCurrentDir.push_back(entry.path());
    }
}

void App::DrawPlaylist()
{
    if (playlistMode == ePlaylistMode::Playlist && _playlist.size() >= 0)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true))
        {
            _selected -= 1;
            if (_selected < 0) _selected = 0;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true))
        {
            _selected += 1;
            if (_selected >= _playlist.size()) _selected = _playlist.size() - 1;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
        {
            if (_selected >= 0)
            {
                PlayPlaylistItem(_selected);
            }
        }
    }

    // Playlist
    ImGui::BeginChild(23, ImVec2(0, -50.0f), true, ImGuiWindowFlags_NoSavedSettings);
    for (size_t i = 0; i < _playlist.size(); i++)
    {
        auto f = std::filesystem::path(_playlist[i]);

        std::string fn = f.filename().generic_string();

        ImGui::PushID(i);
        if (ImGui::Selectable(fn.c_str(), _selected == i))
        {
            _selected = i;
        }

        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            PlayPlaylistItem(_selected);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    if (ImGui::ImageButton("folder", folderImage, ImVec2(24, 24)))
    {
        playlistMode = ePlaylistMode::FindFile;
        ListFoldersAndFiles();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Open playlist or add songs to the current playlist");
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("search", searchImage, ImVec2(24, 24)))
    {
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Find music online");
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("trash", trashImage, ImVec2(24, 24)))
    {
        _playlist.erase(_playlist.begin() + _selected);
        if (_selected >= _playlist.size()) _selected = _playlist.size() - 1;
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Remove selection from the playlist");
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("floppy", floppyImage, ImVec2(24, 24)))
    {
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Save the playlist to disk");
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("copy-plus", copyPlusImage, ImVec2(24, 24)))
    {
        auto itemToCopy = _playlist[_selected];
        _playlist.insert(_playlist.begin() + _selected + 1, itemToCopy);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Duplicate the playlist selection");
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("shuffle", shuffleImage, ImVec2(24, 24)))
    {
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Shuffle the tracks in the playlist");
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(_selected <= 0);
    if (ImGui::ImageButton("arrow-up", arrowUpImage, ImVec2(24, 24)) && _selected > 0)
    {
        auto itemToMove = _playlist[_selected];
        _playlist.erase(_playlist.begin() + _selected);

        _selected--;

        _playlist.insert(_playlist.begin() + _selected, itemToMove);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Move playlist selection up");
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(_selected >= _playlist.size() - 1);
    if (ImGui::ImageButton("arrow-down", arrowDownImage, ImVec2(24, 24)) && _selected < _playlist.size() - 1)
    {
        auto itemToMove = _playlist[_selected];
        _playlist.erase(_playlist.begin() + _selected);

        _selected++;

        _playlist.insert(_playlist.begin() + _selected, itemToMove);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Move playlist selection down");
    }
    ImGui::EndDisabled();
}

void App::DrawFileSelector()
{
    std::filesystem::path openFolder = {};

    if (playlistMode == ePlaylistMode::FindFile)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true))
        {
            selectedFile -= 1;
            if (selectedFile < 0) selectedFile = 0;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true))
        {
            selectedFile += 1;
            if (selectedFile >= foldersAndFilesInCurrentDir.size()) selectedFile = foldersAndFilesInCurrentDir.size() - 1;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
        {
            if (selectedFile >= 0)
            {
                if (std::filesystem::is_directory(foldersAndFilesInCurrentDir[selectedFile]))
                {
                    openFolder = foldersAndFilesInCurrentDir[selectedFile];
                }
                else
                {
                    OpenSelectedFile();
                }
            }
        }
    }

    ImGui::BeginChild(23, ImVec2(0, -50.0f), true, ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("Open Song or playlist");

    ImGui::Separator();

    int i = 0;

    for (const auto &entry : foldersAndFilesInCurrentDir)
    {
        if (entry.empty()) continue;

        auto file = entry.filename();

        auto fn = file.wstring();
        const std::string s(fn.begin(), fn.end());

        ImGui::PushID(s.c_str());
        if (ImGui::Selectable(s.c_str(), selectedFile == i))
        {
            selectedFile = i;
        }

        if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (std::filesystem::is_directory(entry))
            {
                openFolder = entry;
            }
            else
            {
                OpenSelectedFile();
            }
        }

        ImGui::PopID();

        i++;
    }

    ImGui::EndChild();

    if (ImGui::Button("Open"))
    {
        OpenSelectedFile();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        playlistMode = ePlaylistMode::Playlist;
    }

    if (!openFolder.empty())
    {
        findFileStartDir = std::filesystem::canonical(openFolder);
        ListFoldersAndFiles();
        selectedFile = -1;

        openFolder.clear();
    }
}

void App::DrawSettings()
{
    // Playlist
    ImGui::BeginChild("settings", ImVec2(0, -50.0f), true, ImGuiWindowFlags_NoSavedSettings);
    {
    }
    ImGui::EndChild();

    if (ImGui::Button("Close"))
    {
        playlistMode = ePlaylistMode::Playlist;
    }
}

void App::EnsurePlaylistVisible()
{
    if (_isCollapsed)
    {
        SetWindowHeight(expandedHeight);
    }

    _isCollapsed = !_isCollapsed;
}

void App::TogglePlaylist()
{
    if (_isCollapsed)
    {
        SetWindowHeight(expandedHeight);
    }
    else
    {
        SetWindowHeight(collapsedHeight);
    }
    _isCollapsed = !_isCollapsed;
}

void App::OpenSelectedFile()
{
    playlistMode = ePlaylistMode::Playlist;

    if (selectedFile < 0) return;

    auto file = _fileRoot / foldersAndFilesInCurrentDir[selectedFile];

    // Validate file exists and is a regular file
    if (!std::filesystem::exists(file))
    {
        printf("Error: File does not exist: %s\n", file.string().c_str());
        return;
    }

    if (!std::filesystem::is_regular_file(file))
    {
        printf("Error: Not a regular file: %s\n", file.string().c_str());
        return;
    }

    auto fn = file.wstring();
    const std::string s(fn.begin(), fn.end());
    std::cout << "Adding to playlist: " << s << std::endl;

    _playlist.push_back(s);
}

void App::PlayPlaylistItem(int index)
{
    if (index < 0 || index >= (int)_playlist.size())
    {
        printf("Error: Invalid playlist index: %d (playlist size: %zu)\n", index, _playlist.size());
        playState = 0;
        return;
    }

    auto playing = _playlist[index];
    _currentPlaying = playing.filename().string();
    _current_playing_index = index;

    sdl_audio_set_dec(_render, 0);

    if (!open_dec(&_dec, playing.string().c_str()))
    {
        printf("Error: Failed to open MP3 file: %s\n", playing.string().c_str());
        _currentPlaying = "Error loading: " + playing.filename().string();
        playState = 0;
        _current_playing_index = -1;
        return;
    }

    // Update the audio stream to match the MP3's sample rate and channels
    sdl_audio_update_stream_format(_render, _dec.mp3d.info.hz, _dec.mp3d.info.channels);

    sdl_audio_set_dec(_render, &_dec);

    playState = 1;
    headerOffset = 0;
}

void App::OnSongEnded(void *userdata)
{
    App *app = static_cast<App *>(userdata);

    // Auto-advance to next song
    if (_current_playing_index >= 0 && _playlist.size() > 0)
    {
        int attempts = 0;
        const int max_attempts = _playlist.size();

        while (attempts < max_attempts)
        {
            _current_playing_index = (_current_playing_index + 1) % _playlist.size();
            auto playing = _playlist[_current_playing_index];

            sdl_audio_set_dec(_render, 0);

            if (open_dec(&_dec, playing.string().c_str()))
            {
                // Successfully opened the file
                sdl_audio_update_stream_format(_render, _dec.mp3d.info.hz, _dec.mp3d.info.channels);
                sdl_audio_set_dec(_render, &_dec);

                // Update UI to match current song
                if (app)
                {
                    app->_selected = _current_playing_index;
                    app->_currentPlaying = playing.filename().string();
                    app->headerOffset = 0;
                }
                break;
            }
            else
            {
                // Failed to open, try next song
                printf("Error: Failed to open MP3 file during auto-play: %s\n", playing.string().c_str());
                attempts++;
            }
        }

        if (attempts >= max_attempts)
        {
            // All files failed to open
            printf("Error: All files in playlist failed to open\n");
            _current_playing_index = -1;
            if (app)
            {
                app->playState = 0;
                app->_currentPlaying = "No playable files";
            }
        }
    }
}

void App::OnExit()
{
}
