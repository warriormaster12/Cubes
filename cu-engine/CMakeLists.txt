cmake_minimum_required(VERSION 3.28.0)
project(cu-engine VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR} src/*.cpp libs/volk/volk.c libs/vma/vk_mem_alloc.cpp libs/spirv_reflect/spirv_reflect.c)

add_library(cu-engine ${SRC})

target_include_directories(cu-engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/glm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/vulkan-headers
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/vma
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/volk
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/spirv_reflect
)

set(BUILD_BULLET3 ON CACHE BOOL "Build Bullet3 library")

add_subdirectory(libs/glfw EXCLUDE_FROM_ALL)
add_subdirectory(libs/vk-bootstrap EXCLUDE_FROM_ALL)
add_subdirectory(libs/fmt EXCLUDE_FROM_ALL)
add_subdirectory(libs/glslang EXCLUDE_FROM_ALL)
add_subdirectory(libs/bullet3 EXCLUDE_FROM_ALL BUILD_BULLET3)

target_link_libraries(
    cu-engine PUBLIC 
    glfw 
    vk-bootstrap 
    fmt 
    glslang
    SPIRV 
    glslang-default-resource-limits
    libbullet3
)
