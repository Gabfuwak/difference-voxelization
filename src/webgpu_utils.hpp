#pragma once

#include <filesystem>
#include <functional>

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "imgui_impl_glfw.h"

namespace utils {

wgpu::Instance createInstance();

wgpu::Adapter requestAdapterSync(const wgpu::Instance& instance);

wgpu::Device requestDeviceSync(
    const wgpu::Instance& instance,
    const wgpu::Adapter& adapter,
    wgpu::DeviceDescriptor deviceDesc = {}
);

std::function<void(wgpu::RequestAdapterStatus, wgpu::Adapter, wgpu::StringView)>
requestAdapterCallbackFromRef(wgpu::Adapter& adapterRef);

std::function<void(wgpu::RequestDeviceStatus, wgpu::Device, wgpu::StringView)>
requestDeviceCallbackFromRef(wgpu::Device& deviceRef);

void uncapturedErrorCallback(
    const wgpu::Device& device,
    wgpu::ErrorType type,
    wgpu::StringView message
);

void deviceLostCallback(
    const wgpu::Device& device,
    wgpu::DeviceLostReason reason,
    wgpu::StringView message
);

void printAdapterLimits(const wgpu::Adapter& adapter);

wgpu::ShaderModule loadShaderModule(
    const wgpu::Device& device,
    const std::filesystem::path& path
);

wgpu::Surface createSurfaceWithPreferredFormat(
    wgpu::Device device,
    GLFWwindow* window,
    uint32_t width, uint32_t height,
    wgpu::PresentMode presentMode = wgpu::PresentMode::Fifo
);

wgpu::Texture getSurfaceTexture(const wgpu::Surface& surface);

}  // namespace utils
