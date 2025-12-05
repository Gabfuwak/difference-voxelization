cmake_minimum_required(VERSION 3.22)
project(WebGpuDawnTemplate)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DAWN_FETCH_DEPENDENCIES ON)
set(DAWN_BUILD_MONOLITHIC_LIBRARY STATIC)
add_subdirectory("external/dawn" EXCLUDE_FROM_ALL)

find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED)

add_compile_definitions(SHADERS_DIR="${CMAKE_SOURCE_DIR}/src/shaders/")

add_executable(main src/main.cpp)

target_include_directories(main PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/external/tinyobjloader
)

target_link_libraries(main PRIVATE
    webgpu_dawn 
    webgpu_glfw 
    glfw 
    Eigen3::Eigen
    ${OpenCV_LIBS}
)
