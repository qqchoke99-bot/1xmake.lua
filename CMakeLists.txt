cmake_minimum_required(VERSION 3.22)
project(realistic_headbob LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Mod metadata (override from command line) ----
set(MOD_ID              "realistic_headbob"      CACHE STRING "Runtime mod id / folder name")
set(MOD_NAME            "Realistic Head Bob"    CACHE STRING "Display name")
set(MOD_AUTHOR          "port"                  CACHE STRING "Author")
set(MOD_VERSION         "1.0.0"                 CACHE STRING "Version")
set(MOD_LIBRARY_NAME    "realistic_headbob"     CACHE STRING "Output .so base name (libXXX.so)")
set(MOD_MINECRAFT_VERSIONS "[]"                 CACHE STRING "JSON array of supported MC versions")
set(MOD_DESCRIPTION     "Realistic FPS-style head bobbing for LeviLauncher (ported from Realistic Bobbing)" CACHE STRING "")

# ---- Fetch preloader-android SDK ----
include(FetchContent)
FetchContent_Declare(
    preloader_android
    GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
    GIT_TAG        0.2.2
)
FetchContent_MakeAvailable(preloader_android)

# ---- Sources ----
add_library(${MOD_LIBRARY_NAME} SHARED
    src/main.cpp
    src/mod/MyMod.cpp
    src/mod/Config.cpp
    src/mod/HeadBob.cpp
    src/config_generator.cpp
)

target_include_directories(${MOD_LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MOD_LIBRARY_NAME} PRIVATE preloader)

# Android specifics
if(ANDROID)
    target_compile_options(${MOD_LIBRARY_NAME} PRIVATE -fPIC -fvisibility=hidden)
    target_link_options(${MOD_LIBRARY_NAME} PRIVATE -Wl,-z,max-page-size=16384)
endif()

# ---- Generate manifest.json ----
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/manifest.json
    @ONLY
)

# ---- Package target ----
add_custom_target(levi_package
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/package/${MOD_ID}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MOD_LIBRARY_NAME}> ${CMAKE_BINARY_DIR}/package/${MOD_ID}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/manifest.json ${CMAKE_BINARY_DIR}/package/${MOD_ID}/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/package/${MOD_ID}/config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/config.json
            ${CMAKE_BINARY_DIR}/config.schema.json
            ${CMAKE_BINARY_DIR}/package/${MOD_ID}/config/
    COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_BINARY_DIR}/${MOD_ID}.levipack
            --format=zip
            -C ${CMAKE_BINARY_DIR}/package
            ${MOD_ID}
    DEPENDS ${MOD_LIBRARY_NAME} generate_config
    COMMENT "Packaging ${MOD_ID}.levipack"
)

# Config generator runs at build time
add_executable(generate_config src/config_generator.cpp src/mod/Config.cpp)
target_include_directories(generate_config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(generate_config PRIVATE HEADBOB_CONFIG_GENERATOR=1)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/config.json ${CMAKE_BINARY_DIR}/config.schema.json
    COMMAND generate_config ${CMAKE_BINARY_DIR}/config.json ${CMAKE_BINARY_DIR}/config.schema.json
    DEPENDS generate_config
    COMMENT "Generating default config.json + schema"
)
add_custom_target(generate_config_files DEPENDS ${CMAKE_BINARY_DIR}/config.json)
add_dependencies(levi_package generate_config_files)
