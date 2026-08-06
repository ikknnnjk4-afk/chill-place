cmake_minimum_required(VERSION 3.22)
project(ChillPlace CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# ── Raylib Android configuration (must be set BEFORE FetchContent) ──────────
if(ANDROID)
    set(PLATFORM        "Android"                 CACHE STRING "" FORCE)
    set(GRAPHICS        "GRAPHICS_API_OPENGL_ES2" CACHE STRING "" FORCE)
    set(BUILD_EXAMPLES  OFF                       CACHE BOOL   "" FORCE)
    set(BUILD_GAMES     OFF                       CACHE BOOL   "" FORCE)
    set(CUSTOMIZE_BUILD OFF                       CACHE BOOL   "" FORCE)
    set(BUILD_SHARED_LIBS OFF                     CACHE BOOL   "" FORCE)
endif()

# ── Fetch raylib 5.5 ────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# Ensure static raylib is PIC so it can be linked into our shared libgame.so
set_target_properties(raylib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── native_app_glue (provides ANativeActivity_onCreate) ─────────────────────
# ANDROID_NDK is set by the Android Gradle Plugin / NDK toolchain
if(DEFINED ANDROID_NDK)
    set(_NDK_ROOT "${ANDROID_NDK}")
elseif(DEFINED CMAKE_ANDROID_NDK)
    set(_NDK_ROOT "${CMAKE_ANDROID_NDK}")
else()
    message(FATAL_ERROR "ANDROID_NDK / CMAKE_ANDROID_NDK not defined")
endif()

set(NATIVE_APP_GLUE_DIR "${_NDK_ROOT}/sources/android/native_app_glue")
if(NOT EXISTS "${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c")
    message(FATAL_ERROR "native_app_glue not found at ${NATIVE_APP_GLUE_DIR}")
endif()

add_library(native_app_glue STATIC
    "${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c"
)
target_include_directories(native_app_glue PUBLIC "${NATIVE_APP_GLUE_DIR}")
set_target_properties(native_app_glue PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── Game shared library ─────────────────────────────────────────────────────
add_library(game SHARED
    main.cpp
    CrashReporter.cpp
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

target_include_directories(game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${NATIVE_APP_GLUE_DIR}
)

# Critical: force the linker to keep ANativeActivity_onCreate
# (otherwise it is stripped and NativeActivity fails to load the .so)
target_link_options(game PRIVATE
    "-uANativeActivity_onCreate"
)

target_link_libraries(game
    raylib
    native_app_glue
    android
    log
    EGL
    GLESv2
    OpenSLES
)

target_compile_options(game PRIVATE
    -Wno-unused-variable
    -Wno-unused-parameter
    -fno-strict-aliasing
)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(game PRIVATE -O2)
endif()
