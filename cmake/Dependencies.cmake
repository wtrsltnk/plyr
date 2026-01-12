
CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG 0.9.9.8
)

CPMAddPackage(
    NAME SDL3
    VERSION 3.4.0
    URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.0.zip
    OPTIONS
        "SDL_STATIC ON"
)

CPMAddPackage(
    NAME imgui
    GIT_TAG v1.92.2b-docking
    GITHUB_REPOSITORY ocornut/imgui
    DOWNLOAD_ONLY True
)

if (imgui_ADDED)
    add_library(imgui
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    )

    target_include_directories(imgui
        PUBLIC
            "include"
            "${imgui_SOURCE_DIR}/"
    )

    target_link_libraries(imgui
        PRIVATE
            SDL3::SDL3-static
    )

    target_compile_options(imgui
        PUBLIC
            -DIMGUI_IMPL_OPENGL_LOADER_GLAD
    )
else()
    message(FATAL_ERROR "IMGUI not found")
endif()
