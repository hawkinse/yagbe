project(yagbe)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${yagbe_SOURCE_DIR}/cmake")
set(BIN_DIR ${yagbe_SOURCE_DIR}/build)
set(CMake_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

file(GLOB MAIN_SOURCE "src/*.h" "src/*.cpp" )
file(GLOB GB_SOURCE "src/gb/*.h" "src/gb/*.cpp")
file(GLOB SDL_SOURCE "src/sdl/*.h" "src/sdl/*.cpp")

source_group ("" FILES ${MAIN_SOURCE})
source_group ("gb" FILES ${GB_SOURCE})
source_group ("sdl" FILES ${SDL_SOURCE})

add_executable(${PROJECT_NAME} ${MAIN_SOURCE} ${GB_SOURCE} ${SDL_SOURCE})

find_package(SDL2 REQUIRED)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIR} ${PROJECT_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARY})
