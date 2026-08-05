cmake_minimum_required(VERSION 3.22)
project(ChillPlace CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# ── Raylib Android configuration ────────────────────────────────────────────
if(ANDROID)
    set(PLATFORM        "Android"                CACHE STRING "" FORCE)
    set(GRAPHICS        "GRAPHICS_API_OPENGL_ES2" CACHE STRING "" FORCE)
    set(BUILD_EXAMPLES  OFF                       CACHE BOOL   "" FORCE)
    set(BUILD_GAMES     OFF                       CACHE BOOL   "" FORCE)
    set(CUSTOMIZE_BUILD OFF                       CACHE BOOL   "" FORCE)
endif()

# ── Fetch raylib 5.0 ─────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# ── Game source files ─────────────────────────────────────────────────────────
add_library(game SHARED
    main.cpp
    GameApp.cpp
    scene/Room.cpp
    scene/Scene.cpp
    ai/GroqClient.cpp
    ui/Menu.cpp
    ui/HUD.cpp
    audio/AudioManager.cpp
    input/InputManager.cpp
    physics/Physics.cpp
)

target_include_directories(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(game
    raylib
    android
    log
    EGL
    GLESv2
    OpenSLES
)

# Suppress warnings for cleaner CI output
target_compile_options(game PRIVATE -Wno-unused-variable -Wno-unused-parameter)
