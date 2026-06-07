cmake_minimum_required(VERSION 3.20)
project(centipede)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try system SDL3_mixer first, fall back to FetchContent if not found
find_package(SDL3_mixer QUIET)
if(NOT SDL3_mixer_FOUND)
    message(STATUS "SDL3_mixer not found via system, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
      SDL3_mixer
      GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
      GIT_TAG release-3.2.0
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL3_mixer)
endif()

# SDL3 and SDL3_image are installed via pacman so find them normally
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)

add_executable(centipede src/main.cpp)

target_link_libraries(centipede
    PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
)

# Copy assets to build directory so the exe can find them
add_custom_command(TARGET centipede POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:centipede>/assets
    COMMENT "Copying assets to build directory"
)