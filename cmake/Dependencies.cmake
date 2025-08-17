
CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG 0.9.9.8
)

CPMAddPackage(
    NAME SDL2
    VERSION 2.0.14
    URL https://www.libsdl.org/release/SDL2-2.0.14.zip
    OPTIONS
        "SDL_SHARED Off"
)

CPMAddPackage(
    NAME bullet3
    GITHUB_REPOSITORY bulletphysics/bullet3
    GIT_TAG 2.89
    OPTIONS
        "USE_DOUBLE_PRECISION Off"
        "USE_GRAPHICAL_BENCHMARK Off"
        "USE_CUSTOM_VECTOR_MATH Off"
        "USE_MSVC_INCREMENTAL_LINKING Off"
        "USE_MSVC_RUNTIME_LIBRARY_DLL On"
        "USE_GLUT Off"
        "BUILD_DEMOS Off"
        "BUILD_CPU_DEMOS Off"
        "BUILD_BULLET3 Off"
        "BUILD_BULLET2_DEMOS Off"
        "BUILD_EXTRAS Off"
        "INSTALL_EXTRA_LIBS Off"
        "BUILD_UNIT_TESTS Off"
        "INSTALL_LIBS On"
)

if (bullet3_ADDED)
  add_library(bullet3 INTERFACE)
  target_include_directories(bullet3 INTERFACE ${bullet3_SOURCE_DIR}/src)
endif()

CPMAddPackage(
  NAME EnTT
  GIT_TAG master
  GITHUB_REPOSITORY skypjack/entt
  # EnTT's CMakeLists screws with configuration options
  DOWNLOAD_ONLY True
)

if (EnTT_ADDED)
  add_library(EnTT INTERFACE)
  target_include_directories(EnTT INTERFACE ${EnTT_SOURCE_DIR}/src)
endif()

CPMAddPackage(
    NAME imgui
    GIT_TAG v1.82
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
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.cpp"
    )

    target_include_directories(imgui
        PUBLIC
            "include"
            "${imgui_SOURCE_DIR}/"
    )

    target_link_libraries(imgui
        PRIVATE
            SDL2-static
    )

    target_compile_options(imgui
        PUBLIC
            -DIMGUI_IMPL_OPENGL_LOADER_GLAD
    )
else()
    message(FATAL_ERROR "IMGUI not found")
endif()
