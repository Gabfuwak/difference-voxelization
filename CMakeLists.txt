cmake_minimum_required(VERSION 3.22)
project(OurVisionProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_compile_definitions(SHADERS_DIR="${CMAKE_SOURCE_DIR}/src/shaders/")
add_compile_definitions(ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")

include(cmake/dawn.cmake)
include(cmake/imgui.cmake)
add_subdirectory(external/tinyobjloader)
add_subdirectory(external/glm)
find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)

add_executable(main src/main.cpp)
target_include_directories(main PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/external/stb
)
target_link_libraries(main PRIVATE
    webgpu_imgui
    webgpu_glfw
    webgpu_dawn
    glfw
    Eigen3::Eigen
    ${OpenCV_LIBS}
    spdlog::spdlog
    tinyobjloader
    glm
)
