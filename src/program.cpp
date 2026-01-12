#include <app.hpp>
#include <config.h>

#include <iostream>
#include <string>
#include <vector>

#include "audio_sdl.h"

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
        App::_playlist.push_back(argv[i]);

    sdl_audio_init(&App::_render, 44100, 2, 0, 0);

    const std::vector<std::string> args(argv + 1, argv + argc);
    App app(args);

    std::cout << APP_NAME << " version " << APP_VERSION << std::endl;

    if (!app.Init())
    {
        std::cout << "Failed to initialize app" << std::endl;

        return 1;
    }

    auto result = app.Run();
    sdl_audio_release(App::_render);

    return result;
}
