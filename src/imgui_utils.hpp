#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>

namespace utils {

void imguiNewFrame();
void imguiInitialize(GLFWwindow* window, wgpu::Device& device, wgpu::Surface& surface);
void imguiRender(wgpu::Device& device, wgpu::Surface& surface);
void imguiRender(wgpu::RenderPassEncoder& pass);

} // namespace utils
