cmake_minimum_required(VERSION 3.16)
project(dinamo_sdl2 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# Locate SDL2 (works on macOS Homebrew sdl2/sdl2-compat and Linux libsdl2-dev / SteamOS)
find_package(SDL2 QUIET)
if(TARGET SDL2::SDL2)
    set(DINAMO_SDL2_TARGET SDL2::SDL2)
elseif(SDL2_FOUND AND DEFINED SDL2_LIBRARIES)
    add_library(dinamo_sdl2_iface INTERFACE)
    target_include_directories(dinamo_sdl2_iface INTERFACE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(dinamo_sdl2_iface INTERFACE ${SDL2_LIBRARIES})
    set(DINAMO_SDL2_TARGET dinamo_sdl2_iface)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    set(DINAMO_SDL2_TARGET PkgConfig::SDL2)
endif()

# Optional libpng for loading shared/assets/gb/*.png directly
find_package(PNG QUIET)

# Link Elements C++ primitives core (EOS SDK system package, local sibling repo, or FetchContent)
find_package(Elements QUIET)
if(TARGET Elements::elements_core)
    message(STATUS "Using system-installed EOS SDK Elements::elements_core")
else()
    if(NOT DEFINED ELEMENTS_DIR)
        get_filename_component(_DEFAULT_ELEMENTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../elements" ABSOLUTE)
        if(EXISTS "${_DEFAULT_ELEMENTS_DIR}/CMakeLists.txt")
            set(ELEMENTS_DIR "${_DEFAULT_ELEMENTS_DIR}")
        endif()
    endif()

    if(DEFINED ELEMENTS_DIR AND EXISTS "${ELEMENTS_DIR}/CMakeLists.txt")
        message(STATUS "Using local elements repository: ${ELEMENTS_DIR}")
        add_subdirectory("${ELEMENTS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/elements_build" EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        message(STATUS "Fetching elements repository from GitHub...")
        FetchContent_Declare(
            elements
            GIT_REPOSITORY https://github.com/williamlagos/elements.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(elements)
    endif()
endif()

get_filename_component(DINAMO_SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared" ABSOLUTE)

add_library(dinamo_game_lib STATIC
    src/SharedConfig.cpp
    src/PngSprites.cpp
    src/SoundEngine.cpp
    src/BitmapFont.cpp
    src/DinamoGame.cpp
)

target_include_directories(dinamo_game_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(dinamo_game_lib PUBLIC
    SDL_MAIN_HANDLED
    DINAMO_DEFAULT_SHARED_DIR="${DINAMO_SHARED_DIR}"
)

target_link_libraries(dinamo_game_lib PUBLIC
    elements_core
    ${DINAMO_SDL2_TARGET}
    Threads::Threads
)

if(PNG_FOUND)
    target_compile_definitions(dinamo_game_lib PUBLIC DINAMO_HAS_PNG=1)
    target_link_libraries(dinamo_game_lib PUBLIC PNG::PNG)
else()
    target_compile_definitions(dinamo_game_lib PUBLIC DINAMO_HAS_PNG=0)
endif()

add_executable(dinamo_sdl2 src/main.cpp)
target_link_libraries(dinamo_sdl2 PRIVATE dinamo_game_lib)

enable_testing()
add_executable(dinamo_smoke_test tests/smoke_test.cpp)
target_link_libraries(dinamo_smoke_test PRIVATE dinamo_game_lib)
add_test(NAME dinamo_smoke COMMAND dinamo_smoke_test)
