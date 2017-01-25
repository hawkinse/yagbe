project(yagbe)

cmake_minimum_required(VERSION 2.8)

set(BIN_DIR ${yagbe_SOURCE_DIR}/build)
set(CMake_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_executable(${PROJECT_NAME} main.cpp SDLAudioPlayer.cpp SDLBufferRenderer.cpp SDLInputChecker.cpp gb/gbz80cpu.cpp gb/gbmem.cpp gb/gbcart.cpp gb/gbpad.cpp gb/gblcd.cpp gb/gbaudio.cpp gb/gbserial.cpp)

INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2MIXER REQUIRED SDL2_mixer>=2.0.0)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR} ${SDL2MIXER_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2MIXER_LIBRARIES})
