cmake_minimum_required(VERSION 3.28.0)
project(cubes VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR} src/*.cpp)
add_executable(cubes ${SRC})
target_include_directories(cubes PUBLIC src/)

#cu-engine
add_subdirectory(cu-engine EXCLUDE_FROM_ALL)

target_link_libraries(cubes PRIVATE cu-engine)
