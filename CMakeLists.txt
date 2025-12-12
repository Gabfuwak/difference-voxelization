cmake_minimum_required(VERSION 3.22)
project(OurVisionProject)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_compile_definitions(SHADERS_DIR="${CMAKE_SOURCE_DIR}/src/shaders/")
add_compile_definitions(ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")

include(cmake/dawn.cmake)
include(cmake/imgui.cmake)
add_subdirectory(external/tinyobjloader)
add_subdirectory(external/glm)
find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB BIN_TARGETS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/bin/*.cpp")

function(add_wgpu_executable name file)
    add_executable(${name} ${file} ${SRC_FILES})
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src ${OpenCV_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/external/stb)
    target_compile_definitions(${name} PRIVATE SHADERS_DIR="${CMAKE_SOURCE_DIR}/src/shaders/")
    target_compile_definitions(${name} PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")
    target_link_libraries(${name} PRIVATE
        webgpu_imgui
        webgpu_dawn
        webgpu_glfw
        glfw
        tinyobjloader
        glm
        Eigen3::Eigen
        ${OpenCV_LIBS}
        spdlog::spdlog
    )
endfunction()

foreach(bin_target ${BIN_TARGETS})
    get_filename_component(bin_target_name ${bin_target} NAME_WE)
    add_wgpu_executable(${bin_target_name} ${bin_target})
endforeach()
