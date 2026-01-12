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

void App::OnInit()
{
    glClearColor(0.56f, 0.7f, 0.67f, 1.0f);
    glEnable(GL_DEPTH_TEST);
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
    headerOffset += (diff.count() / 10000000.0f);

    progress = (float(_dec.mp3d.cur_sample) / float(_dec.mp3d.samples));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static bool is_open = true;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(_width, _height));
    ImGui::Begin("Debug", &is_open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    DrawTitleTicker();

    DrawPlaybackControls();

    ImGui::SameLine();

    DrawClock();

    DrawTimeline();

    if (_height > 400)
    {
        if (!findingFile)
        {
            DrawPlaylist();
        }
        else
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
}

void App::DrawTitleTicker()
{
    ImGui::PushFont(header_font);
    auto f = std::filesystem::path(_currentPlaying).replace_extension();
    std::string fn = f.filename().generic_string();

    if (fn.size() == 0)
    {
        fn = "...";
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
    const int channelCount = 2;
    const int sampleRate = 44100;
    auto currentSeconds = (_dec.mp3d.cur_sample / channelCount) / sampleRate;
    auto totalSeconds = (_dec.mp3d.samples / channelCount) / sampleRate;

    auto currentMinutes = int(std::floor(currentSeconds / 60.0));
    auto totalMinutes = int(std::floor(totalSeconds / 60.0));

    char buf[256];
    sprintf_s(buf, 256, "%02d:%02d:%02d / %02d:%02d:%02d",
              int(std::floor(currentMinutes / 60)),
              currentMinutes % 60,
              int(currentSeconds % 60),
              int(std::floor(totalMinutes / 60)),
              totalMinutes % 60,
              int(totalSeconds % 60));

    ImGui::Text("%s", buf);
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
        findingFile = true;
        if (_playlist.size() > 0)
        {
            findFileStartDir = std::filesystem::path(_playlist[_selected]).parent_path();
        }

        filesInCurrentDir.clear();
        auto diritr = std::filesystem::directory_iterator("C:\\Users\\woute\\Music");
        for (const auto &entry : diritr)
        {
            if (!entry.is_regular_file()) continue;

            filesInCurrentDir.push_back(entry.path());
        }
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
    ImGui::BeginChild(23, ImVec2(0, -50.0f), true);

    ImGui::Text("Open Song or playlist");

    ImGui::Separator();

    int i = 0;

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
        }

        ImGui::PopID();

        i++;
    }
    ImGui::EndChild();

    if (ImGui::Button("Open"))
    {
        findingFile = false;
        if (selectedFile >= 0)
        {
            auto file = std::filesystem::path("C:\\Users\\woute\\Music") / filesInCurrentDir[selectedFile];

            auto fn = file.wstring();
            const std::string s(fn.begin(), fn.end());
            std::cout << s << std::endl;

            _playlist.push_back(s);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        findingFile = false;
    }
}

void App::PlayPlaylistItem(int index)
{
    auto playing = _playlist[index];
    _currentPlaying = playing.filename().string();
    sdl_audio_set_dec(_render, 0);
    open_dec(&_dec, playing.string().c_str());

    // Update the audio stream to match the MP3's sample rate and channels
    sdl_audio_update_stream_format(_render, _dec.mp3d.info.hz, _dec.mp3d.info.channels);

    sdl_audio_set_dec(_render, &_dec);

    playState = 1;
    headerOffset = 0;
}

void App::OnExit()
{
}
