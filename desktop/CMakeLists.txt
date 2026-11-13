cmake_minimum_required(VERSION 3.18)
project(dinamo_desktop VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ELEMENTS "Link optional elements C++ library if available" OFF)
option(ENABLE_STEAMWORKS "Link optional Valve Steamworks SDK" OFF)
option(ENABLE_SAMEBOY_FETCH "Fetch SameBoy core via CMake FetchContent" ON)

# --- 1. SDL2 ---
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
endif()

# --- 2. SameBoy MIT Core ---
set(SAMEBOY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/sameboy" CACHE PATH "Path to SameBoy source")
if(NOT EXISTS "${SAMEBOY_DIR}/Core/gb.c")
    if(NOT ENABLE_SAMEBOY_FETCH)
        message(FATAL_ERROR "SameBoy core not found in ${SAMEBOY_DIR} and ENABLE_SAMEBOY_FETCH=OFF")
    endif()
    include(FetchContent)
    FetchContent_Declare(
        sameboy
        URL https://github.com/LIJI32/SameBoy/archive/ea667ad79aa28409f034b1fd6da7304e1844750d.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(sameboy)
    set(SAMEBOY_DIR "${sameboy_SOURCE_DIR}")
endif()

set(SAMEBOY_CORE_SOURCES
    "${SAMEBOY_DIR}/Core/gb.c"
    "${SAMEBOY_DIR}/Core/sgb.c"
    "${SAMEBOY_DIR}/Core/apu.c"
    "${SAMEBOY_DIR}/Core/memory.c"
    "${SAMEBOY_DIR}/Core/mbc.c"
    "${SAMEBOY_DIR}/Core/timing.c"
    "${SAMEBOY_DIR}/Core/display.c"
    "${SAMEBOY_DIR}/Core/camera.c"
    "${SAMEBOY_DIR}/Core/sm83_cpu.c"
    "${SAMEBOY_DIR}/Core/joypad.c"
    "${SAMEBOY_DIR}/Core/save_state.c"
    "${SAMEBOY_DIR}/Core/random.c"
    "${SAMEBOY_DIR}/Core/rumble.c"
)

add_library(sameboy_core STATIC ${SAMEBOY_CORE_SOURCES})
target_include_directories(sameboy_core PUBLIC "${SAMEBOY_DIR}")
target_compile_definitions(sameboy_core PUBLIC
    GB_DISABLE_TIMEKEEPING
    GB_DISABLE_REWIND
    GB_DISABLE_DEBUGGER
    GB_DISABLE_CHEATS
)
target_compile_definitions(sameboy_core PRIVATE
    GB_INTERNAL
    GB_VERSION="1.0.2"
    GB_COPYRIGHT_YEAR="2026"
    _GNU_SOURCE
    _USE_MATH_DEFINES
)
if(UNIX)
    target_link_libraries(sameboy_core PUBLIC m)
endif()

# --- 3. Thin C11 Bridge for SameBoy Core ---
add_library(sameboy_host STATIC src/sameboy_host.c)
target_include_directories(sameboy_host PUBLIC src)
target_link_libraries(sameboy_host PRIVATE sameboy_core)

# --- 4. Dinamo Desktop / Steam Deck Executable ---
add_executable(dinamo-desktop
    src/main.cpp
    src/palettes.cpp
)
target_include_directories(dinamo-desktop PRIVATE src)
target_link_libraries(dinamo-desktop PRIVATE sameboy_host SDL2::SDL2)
if(TARGET SDL2::SDL2main)
    target_link_libraries(dinamo-desktop PRIVATE SDL2::SDL2main)
endif()

# Optional elements library bridge (prepares for future Option B migration or Steam leaderboards)
if(ENABLE_ELEMENTS)
    set(ELEMENTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../elements" CACHE PATH "Path to elements library")
    if(EXISTS "${ELEMENTS_DIR}/CMakeLists.txt")
        add_subdirectory("${ELEMENTS_DIR}" "${CMAKE_BINARY_DIR}/elements_build" EXCLUDE_FROM_ALL)
        target_link_libraries(dinamo-desktop PRIVATE elements_core)
        target_compile_definitions(dinamo-desktop PRIVATE DINAMO_HAS_ELEMENTS=1)
    else()
        message(WARNING "ENABLE_ELEMENTS=ON but ${ELEMENTS_DIR} not found")
    endif()
endif()

# Optional Valve Steamworks SDK bridge
if(ENABLE_STEAMWORKS)
    set(STEAMWORKS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/steamworks" CACHE PATH "Path to Steamworks SDK redistributable_bin")
    if(EXISTS "${STEAMWORKS_DIR}/public/steam/steam_api.h")
        target_include_directories(dinamo-desktop PRIVATE "${STEAMWORKS_DIR}/public")
        target_compile_definitions(dinamo-desktop PRIVATE DINAMO_HAS_STEAMWORKS=1)
        if(APPLE)
            find_library(STEAM_API_LIB steam_api PATHS "${STEAMWORKS_DIR}/redistributable_bin/osx" REQUIRED)
        elseif(UNIX)
            find_library(STEAM_API_LIB steam_api PATHS "${STEAMWORKS_DIR}/redistributable_bin/linux64" REQUIRED)
        elseif(WIN32)
            find_library(STEAM_API_LIB steam_api64 PATHS "${STEAMWORKS_DIR}/redistributable_bin/win64" REQUIRED)
        endif()
        target_link_libraries(dinamo-desktop PRIVATE "${STEAM_API_LIB}")
    else()
        message(WARNING "ENABLE_STEAMWORKS=ON but Steamworks SDK not found in ${STEAMWORKS_DIR}")
    endif()
endif()

# --- 5. Single-Source Shared Palettes ---
set(DINAMO_SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared")
if(EXISTS "${DINAMO_SHARED_DIR}/palettes.json")
    add_custom_command(TARGET dinamo-desktop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:dinamo-desktop>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DINAMO_SHARED_DIR}/palettes.json"
            "$<TARGET_FILE_DIR:dinamo-desktop>/assets/palettes.json"
        COMMENT "Syncing shared/palettes.json to output assets/"
    )
endif()

# --- 6. CTest Smoke Verification ---
enable_testing()
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(
        NAME dinamo_desktop_headless_smoke
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_smoke_test.py"
            "$<TARGET_FILE:dinamo-desktop>"
            "${CMAKE_BINARY_DIR}/smoke_out"
    )
endif()
