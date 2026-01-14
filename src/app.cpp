#include <app.hpp>

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

void App::OnInit()
{
    glClearColor(0.56f, 0.7f, 0.67f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // Set up auto-play callback with this App instance
    sdl_audio_set_end_callback(_render, &App::OnSongEnded, this);
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
    }

    static bool is_open = true;

    // Always position ImGui window at (0,0) to fill the SDL window
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(_width, _height));
    ImGui::Begin("plyr", &is_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

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
        if (ImGui::Button(ICON_LC_PAUSE))
        {
            sdl_audio_pause(_render, 1);
            playState = 2;
        }
    }
    else if (playState == 2)
    {
        if (ImGui::Button(ICON_LC_PLAY))
        {
            sdl_audio_pause(_render, 0);
            playState = 1;
        }
    }
    else
    {
        if (ImGui::Button(ICON_LC_PLAY))
        {
            PlayPlaylistItem(_selected);
        }
    }
    ImGui::SameLine();

    if (ImGui::Button(ICON_LC_SQUARE))
    {
        sdl_audio_set_dec(_render, 0);
        playState = 0;
        mp3dec_ex_seek(&_dec.mp3d, 0);
    }
    ImGui::SameLine();

    if (ImGui::Button(ICON_LC_SKIP_BACK))
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

    if (ImGui::Button(ICON_LC_SKIP_FORWARD))
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

    if (ImGui::Button(ICON_LC_LIST_MUSIC))
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

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Show/hide the playlist");
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
    foldersInCurrentDir.clear();
    filesInCurrentDir.clear();

    auto diritr = std::filesystem::directory_iterator(findFileStartDir);

    if (std::filesystem::canonical(findFileStartDir).compare(_fileRoot) > 0)
    {
        foldersInCurrentDir.push_back(findFileStartDir / "..");
    }

    for (const auto &entry : diritr)
    {
        if (entry.is_directory())
        {
            foldersInCurrentDir.push_back(entry.path());
        }
        if (entry.is_regular_file())
        {
            filesInCurrentDir.push_back(entry.path());
        }
    }
}

void App::DrawPlaylist()
{
    // Playlist
    ImGui::BeginChild(23, ImVec2(0, -50.0f), true);
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

    if (ImGui::Button(ICON_LC_FOLDER))
    {
        selectedFile = -1;
        playlistMode = ePlaylistMode::FindFile;

        findFileStartDir = _fileRoot;
        ListFoldersAndFiles();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Open playlist or add songs to the current playlist");
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_LC_TRASH_2))
    {
        _playlist.erase(_playlist.begin() + _selected);
        if (_selected >= _playlist.size()) _selected = _playlist.size() - 1;
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Remove selection from the playlist");
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_LC_SAVE))
    {
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Save the playlist to disk");
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_LC_COPY))
    {
        auto itemToCopy = _playlist[_selected];
        _playlist.insert(_playlist.begin() + _selected + 1, itemToCopy);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Duplicate the playlist selection");
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_LC_SHUFFLE))
    {
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Shuffle the tracks in the playlist");
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(_selected <= 0);
    if (ImGui::Button(ICON_LC_ARROW_UP) && _selected > 0)
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
    if (ImGui::Button(ICON_LC_ARROW_DOWN) && _selected < _playlist.size() - 1)
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

    ImGui::BeginChild(23, ImVec2(0, -50.0f), true);

    ImGui::Text("Open Song or playlist");

    ImGui::Separator();

    int i = 0;

    for (const auto &entry : foldersInCurrentDir)
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
            openFolder = entry;
        }

        ImGui::PopID();

        i++;
    }

    i = 0;

    for (const auto &entry : filesInCurrentDir)
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
            OpenSelectedFile();
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

        openFolder.clear();
    }
}

void App::OpenSelectedFile()
{
    playlistMode = ePlaylistMode::Playlist;

    if (selectedFile < 0) return;

    auto file = _fileRoot / filesInCurrentDir[selectedFile];

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

void App::OnExit()
{
}
