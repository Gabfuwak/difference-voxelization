cmake_minimum_required(VERSION 3.22)
project(WebGpuDawnTemplate)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DAWN_FETCH_DEPENDENCIES ON)
set(DAWN_BUILD_MONOLITHIC_LIBRARY STATIC)
add_subdirectory("external/dawn" EXCLUDE_FROM_ALL)

add_executable(main src/main.cpp)

if (EMSCRIPTEN)
    set_target_properties(main PROPERTIES SUFFIX ".html")
    target_link_libraries(main PRIVATE emdawnwebgpu_cpp webgpu_glfw)
    target_link_options(main PRIVATE "-sASYNCIFY=1" "-sUSE_GLFW=3")
else()
    target_link_libraries(main PRIVATE webgpu_dawn webgpu_glfw glfw)
endif()
