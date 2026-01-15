cmake_minimum_required(VERSION 3.21)

project(plyr LANGUAGES CXX C VERSION "0.2.0")

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
    src/default-icons.cpp
)

if (WIN32)
    target_sources(plyr PRIVATE resources/app.rc)
endif()

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
        "thirdparty/minimp3/include"
        "thirdparty/tomykaira"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(plyr
    PRIVATE
        glm
        imgui
        SDL3::SDL3-static
)

# Test/diagnostic executables
add_executable(check_file
    src/check_file.c
)

add_executable(check_after_id3
    src/check_after_id3.c
)

add_executable(make_base64_image_header
    src/make_base64_image_header.cpp
)

target_compile_features(make_base64_image_header
    PRIVATE
        cxx_nullptr
        cxx_std_23
)

target_include_directories(make_base64_image_header
    PRIVATE
        "include"
        "thirdparty/tomykaira"
        "${PROJECT_BINARY_DIR}"
)
