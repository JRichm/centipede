cmake_minimum_required(VERSION 3.20)
project(centipede)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
# Find SDL3 and its extension libraries
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_mixer REQUIRED)
 
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
 