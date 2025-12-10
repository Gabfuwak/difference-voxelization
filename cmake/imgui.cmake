set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(webgpu_imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
)

target_include_directories(webgpu_imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(webgpu_imgui PUBLIC glfw webgpu_dawn)
target_compile_definitions(webgpu_imgui PUBLIC "IMGUI_IMPL_WEBGPU_BACKEND_DAWN")

if(APPLE)
    set_source_files_properties(${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()
