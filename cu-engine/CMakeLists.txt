cmake_minimum_required(VERSION 3.28.0)
project(cu-engine VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR} src/*.cpp libs/volk/volk.c libs/vma/vk_mem_alloc.cpp)

add_library(cu-engine ${SRC})

target_include_directories(cu-engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/private
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs/vulkan-headers
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs/vma
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs/volk
)

add_subdirectory(libs/glfw EXCLUDE_FROM_ALL)
add_subdirectory(libs/vk-bootstrap EXCLUDE_FROM_ALL)
add_subdirectory(libs/fmt EXCLUDE_FROM_ALL)

target_link_libraries(cu-engine PRIVATE glfw vk-bootstrap fmt)
