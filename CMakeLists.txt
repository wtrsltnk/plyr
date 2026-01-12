cmake_minimum_required(VERSION 3.14)

project(plyr VERSION "0.2.0")

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

configure_file(config.h.in config.h)

add_executable(plyr
    include/app.hpp
    include/entities.hpp
    include/glprogram.hpp
    include/glshader.hpp
    include/vertexarray.hpp
    src/app-infra.cpp
    src/app.cpp
    src/audio_sdl.c
    src/audio_sdl.h
    src/decode.c
    src/decode.h
    src/glad.c
    src/program.cpp
    src/vertexarray.cpp
)

target_compile_features(plyr
    PRIVATE
        cxx_nullptr
        cxx_std_23
)

target_compile_definitions(plyr
    PRIVATE
        UNICODE
)

target_include_directories(plyr
    PRIVATE
        "include"
        "thirdparty/IconFontCppHeaders"
        "thirdparty/minimp3/include"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(plyr
    PRIVATE
        glm
        imgui
        SDL3::SDL3-static
)

