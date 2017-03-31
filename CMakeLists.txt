project(yagbe)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${yagbe_SOURCE_DIR}/cmake")
set(BIN_DIR ${yagbe_SOURCE_DIR}/build)
set(CMake_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_executable(${PROJECT_NAME} main.cpp SDLAudioPlayer.cpp SDLBufferRenderer.cpp SDLInputChecker.cpp gb/gbz80cpu.cpp gb/gbmem.cpp gb/gbcart.cpp gb/gbpad.cpp gb/gblcd.cpp gb/gbaudio.cpp gb/gbserial.cpp)

find_package(SDL2 REQUIRED)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARY})