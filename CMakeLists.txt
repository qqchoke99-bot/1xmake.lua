cmake_minimum_required(VERSION 3.22)
project(realistic_headbob LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Mod metadata
# ============================================================

set(MOD_ID
    "realistic_headbob"
    CACHE STRING "Runtime mod id / folder name"
)

set(MOD_NAME
    "Realistic Head Bob"
    CACHE STRING "Display name"
)

set(MOD_AUTHOR
    "port"
    CACHE STRING "Author"
)

set(MOD_VERSION
    "1.0.0"
    CACHE STRING "Version"
)

set(MOD_LIBRARY_NAME
    "realistic_headbob"
    CACHE STRING "Output .so base name"
)

set(MOD_MINECRAFT_VERSIONS
    "[]"
    CACHE STRING "JSON array of supported MC versions"
)

set(MOD_DESCRIPTION
    "Realistic FPS-style head bobbing for LeviLauncher (ported from Realistic Bobbing)"
    CACHE STRING ""
)

# ============================================================
# Fetch preloader-android SDK
# ============================================================

include(FetchContent)

FetchContent_Declare(
    preloader_android
    GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
    GIT_TAG 0.2.2
)

FetchContent_MakeAvailable(preloader_android)

# ============================================================
# Mod library
# ============================================================

add_library(${MOD_LIBRARY_NAME} SHARED
    src/main.cpp
    src/mod/MyMod.cpp
    src/mod/Config.cpp
    src/mod/HeadBob.cpp
)

target_include_directories(${MOD_LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# preloader Logger.hpp uses Android __android_log_print()
target_link_libraries(${MOD_LIBRARY_NAME} PRIVATE
    preloader
    log
)

# Android specifics
if(ANDROID)
    target_compile_options(${MOD_LIBRARY_NAME} PRIVATE
        -fPIC
        -fvisibility=hidden
    )

    target_link_options(${MOD_LIBRARY_NAME} PRIVATE
        -Wl,-z,max-page-size=16384
    )
endif()

# ============================================================
# Generate manifest.json
# ============================================================

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/manifest.json
    @ONLY
)

# ============================================================
# Generate config.json directly with CMake
# ============================================================

file(WRITE
    ${CMAKE_CURRENT_BINARY_DIR}/config.json
[=[
{
  "enabled": true,
  "idleEnabled": true,
  "enableWhenMounted": true,
  "enableThirdPerson": false,
  "mode": "bodycam",
  "globalStrength": 0.32,
  "walkMult": 1.0,
  "runMult": 1.0,
  "stepStrength": 1.0,
  "stepRate": 1.0,
  "swayStrength": 1.0,
  "cornerStrength": 1.0,
  "breathStrength": 1.0,
  "breathRate": 1.0,
  "driftStrength": 1.0,
  "driftRate": 1.0,
  "idleShakeMult": 1.0,
  "smoothHz": 1.75,
  "damping": 1.25,
  "fadeLerp": 0.15,
  "moduleName": "libminecraftpe.so",
  "cameraBlendSig": "F4 4F 06 A9 FD 03 01 91 54 D0 3B D5 F3 03 00 AA 88 ?? ?? F9 A8 83 1F F8 ?? ?? ?? D0 08 A1 11 91 08 FD DF 08 ?? ?? ?? ?? ?? ?? ?? D0 21 E0 0C 91 E0 63 00 91 ?? ?? ?? 95",
  "localPlayerTickSig": ""
}
]=]
)

file(WRITE
    ${CMAKE_CURRENT_BINARY_DIR}/config.schema.json
[=[
{
  "title": "Realistic Head Bob",
  "type": "object",
  "properties": {
    "enabled": {
      "type": "boolean",
      "default": true
    },
    "mode": {
      "type": "string",
      "enum": [
        "default",
        "bodycam",
        "comfort",
        "custom"
      ],
      "default": "bodycam"
    },
    "globalStrength": {
      "type": "number",
      "minimum": 0,
      "maximum": 3,
      "default": 0.32
    },
    "stepStrength": {
      "type": "number",
      "minimum": 0,
      "maximum": 3,
      "default": 1.0
    },
    "swayStrength": {
      "type": "number",
      "minimum": 0,
      "maximum": 3,
      "default": 1.0
    },
    "smoothHz": {
      "type": "number",
      "minimum": 0.25,
      "maximum": 6,
      "default": 1.75
    },
    "damping": {
      "type": "number",
      "minimum": 0.1,
      "maximum": 3,
      "default": 1.25
    },
    "moduleName": {
      "type": "string",
      "default": "libminecraftpe.so"
    },
    "cameraBlendSig": {
      "type": "string",
      "default": ""
    },
    "localPlayerTickSig": {
      "type": "string",
      "default": ""
    }
  }
}
]=]
)

# ============================================================
# Package target
# ============================================================

add_custom_target(levi_package

    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/package/${MOD_ID}

    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${MOD_LIBRARY_NAME}>
        ${CMAKE_BINARY_DIR}/package/${MOD_ID}/

    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/manifest.json
        ${CMAKE_BINARY_DIR}/package/${MOD_ID}/

    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_BINARY_DIR}/package/${MOD_ID}/config

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/config.json
        ${CMAKE_BINARY_DIR}/config.schema.json
        ${CMAKE_BINARY_DIR}/package/${MOD_ID}/config/

    # --------------------------------------------------------
    # Create ZIP/LEVIPACK.
    #
    # cmake -E tar does NOT support normal tar "-C" syntax
    # in this environment.
    #
    # Instead change directory using cmake -E chdir first.
    # --------------------------------------------------------

    COMMAND ${CMAKE_COMMAND} -E chdir
        ${CMAKE_BINARY_DIR}/package
        ${CMAKE_COMMAND} -E tar
        cf
        ${CMAKE_BINARY_DIR}/${MOD_ID}.levipack
        --format=zip
        ${MOD_ID}

    DEPENDS
        ${MOD_LIBRARY_NAME}

    COMMENT "Packaging ${MOD_ID}.levipack"
)