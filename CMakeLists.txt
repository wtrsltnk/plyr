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
    include/iphysicsservice.hpp
    include/vertexarray.hpp
    src/app-infra.cpp
    src/app.cpp
    src/glad.c
    src/services/physicsservice.cpp
    src/services/physicsservice.hpp
    src/program.cpp
    src/vertexarray.cpp
)

target_compile_features(plyr
    PRIVATE
        cxx_nullptr
        cxx_std_17  # EnTT needs this
)

target_include_directories(plyr
    PRIVATE
        "include"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(plyr
    PRIVATE
        glm
        imgui
        SDL2-static
        BulletDynamics
        BulletCollision
        BulletSoftBody
        LinearMath
        bullet3
        EnTT

)

