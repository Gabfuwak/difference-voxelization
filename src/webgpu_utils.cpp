#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>
#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>

#include "webgpu_utils.hpp"
#include "utils.hpp"

using namespace wgpu;
namespace fs = std::filesystem;

namespace utils {

Instance createInstance() {
    std::array requiredFeatures {
        InstanceFeatureName::TimedWaitAny
    };
    InstanceDescriptor instanceDesc {
        .requiredFeatureCount = requiredFeatures.size(),
        .requiredFeatures = requiredFeatures.data(),
    };
    return CreateInstance(&instanceDesc);
}

Adapter requestAdapterSync(const Instance& instance) {
    Adapter adapter;

    RequestAdapterOptions options {};
    auto callback = requestAdapterCallbackFromRef(adapter);
    auto f = instance.RequestAdapter(&options, CallbackMode::WaitAnyOnly, callback);
    instance.WaitAny(f, UINT64_MAX);

    return adapter;
}

Device requestDeviceSync(
    const Instance& instance,
    const Adapter& adapter,
    DeviceDescriptor deviceDesc
) {
    Device device;

    auto callbackMode = CallbackMode::WaitAnyOnly;
    auto callback = requestDeviceCallbackFromRef(device);

    deviceDesc.SetDeviceLostCallback(CallbackMode::WaitAnyOnly, deviceLostCallback);
    deviceDesc.SetUncapturedErrorCallback(uncapturedErrorCallback);

    auto future = adapter.RequestDevice(&deviceDesc, callbackMode, callback);
    instance.WaitAny(future, UINT64_MAX);

    return device;
}

std::function<void(RequestAdapterStatus, Adapter, StringView)>
requestAdapterCallbackFromRef(Adapter& adapterRef) {
    return [&adapterRef](
        RequestAdapterStatus status,
        Adapter adapter,
        StringView message
    ) {
        if (status != RequestAdapterStatus::Success) {
            std::cerr << "RequestDevice: " << message << '\n';
            std::exit(1);
        }
        adapterRef = std::move(adapter);
    };
}

std::function<void(RequestDeviceStatus, Device, StringView)>
requestDeviceCallbackFromRef(Device& deviceRef) {
    return [&deviceRef](
        RequestDeviceStatus status,
        Device device,
        StringView message
    ) {
        if (status != RequestDeviceStatus::Success) {
            std::cerr << "RequestDevice: " << message << '\n';
            std::exit(1);
        }
        deviceRef = std::move(device);
    };
}

void uncapturedErrorCallback(
    const Device& device,
    ErrorType type,
    StringView message
) {
    std::ostringstream oss;
    oss << "Error: " << type << " - message: " << message;
    spdlog::error(oss.str());
    throw std::runtime_error("WGPU Error");
}

void deviceLostCallback(
    const Device&,
    DeviceLostReason reason,
    StringView message
) {
    std::ostringstream oss;
    oss << "DeviceLost: " << reason << " - message: " << message << '\n';
    spdlog::info(oss.str());

}

void printAdapterLimits(const Adapter& adapter) {
    Limits limits;
    adapter.GetLimits(&limits);

    std::cout << "maxTextureDimension1D: " << limits.maxTextureDimension1D << '\n';
    std::cout << "maxTextureDimension2D: " << limits.maxTextureDimension2D << '\n';
    std::cout << "maxTextureDimension3D: " << limits.maxTextureDimension3D << '\n';
    std::cout << "maxTextureArrayLayers: " << limits.maxTextureArrayLayers << '\n';
    std::cout << "maxBindGroups: " << limits.maxBindGroups << '\n';
    std::cout << "maxBindGroupsPlusVertexBuffers: " << limits.maxBindGroupsPlusVertexBuffers << '\n';
    std::cout << "maxBindingsPerBindGroup: " << limits.maxBindingsPerBindGroup << '\n';
    std::cout << "maxDynamicUniformBuffersPerPipelineLayout: " << limits.maxDynamicUniformBuffersPerPipelineLayout << '\n';
    std::cout << "maxDynamicStorageBuffersPerPipelineLayout: " << limits.maxDynamicStorageBuffersPerPipelineLayout << '\n';
    std::cout << "maxSampledTexturesPerShaderStage: " << limits.maxSampledTexturesPerShaderStage << '\n';
    std::cout << "maxSamplersPerShaderStage: " << limits.maxSamplersPerShaderStage << '\n';
    std::cout << "maxStorageBuffersPerShaderStage: " << limits.maxStorageBuffersPerShaderStage << '\n';
    std::cout << "maxStorageTexturesPerShaderStage: " << limits.maxStorageTexturesPerShaderStage << '\n';
    std::cout << "maxUniformBuffersPerShaderStage: " << limits.maxUniformBuffersPerShaderStage << '\n';
    std::cout << "maxUniformBufferBindingSize: " << limits.maxUniformBufferBindingSize << '\n';
    std::cout << "maxStorageBufferBindingSize: " << limits.maxStorageBufferBindingSize << '\n';
    std::cout << "minUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment << '\n';
    std::cout << "minStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment << '\n';
    std::cout << "maxVertexBuffers: " << limits.maxVertexBuffers << '\n';
    std::cout << "maxBufferSize: " << limits.maxBufferSize << '\n';
    std::cout << "maxVertexAttributes: " << limits.maxVertexAttributes << '\n';
    std::cout << "maxVertexBufferArrayStride: " << limits.maxVertexBufferArrayStride << '\n';
    std::cout << "maxInterStageShaderVariables: " << limits.maxInterStageShaderVariables << '\n';
    std::cout << "maxColorAttachments: " << limits.maxColorAttachments << '\n';
    std::cout << "maxColorAttachmentBytesPerSample: " << limits.maxColorAttachmentBytesPerSample << '\n';
    std::cout << "maxComputeWorkgroupStorageSize: " << limits.maxComputeWorkgroupStorageSize << '\n';
    std::cout << "maxComputeInvocationsPerWorkgroup: " << limits.maxComputeInvocationsPerWorkgroup << '\n';
    std::cout << "maxComputeWorkgroupSizeX: " << limits.maxComputeWorkgroupSizeX << '\n';
    std::cout << "maxComputeWorkgroupSizeY: " << limits.maxComputeWorkgroupSizeY << '\n';
    std::cout << "maxComputeWorkgroupSizeZ: " << limits.maxComputeWorkgroupSizeZ << '\n';
    std::cout << "maxComputeWorkgroupsPerDimension: " << limits.maxComputeWorkgroupsPerDimension << '\n';
    std::cout << "maxImmediateSize: " << limits.maxImmediateSize << '\n';
}

ShaderModule loadShaderModule(const Device& device, const fs::path& filename) {
    auto path = findShaderPath(filename);
    spdlog::info("Loading shader from {}", path.string());
    auto code = readFile(SHADERS_DIR / path);
    ShaderSourceWGSL shaderSource {{.code = code.data()}};
    ShaderModuleDescriptor shaderModuleDesc {.nextInChain = &shaderSource};
    auto shaderModule = device.CreateShaderModule(&shaderModuleDesc);

    return shaderModule;
}

Surface createSurfaceWithPreferredFormat(
    Device& device,
    GLFWwindow* window
) {
    auto adapter = device.GetAdapter();
    auto instance = adapter.GetInstance();
    auto surface = glfw::CreateSurfaceForWindow(instance, window);

    SurfaceCapabilities capabilities {};
    surface.GetCapabilities(adapter, &capabilities);
    auto defaultFormat = capabilities.formats[0];

    int fWidth, fHeight;
    glfwGetFramebufferSize(window, &fWidth, &fHeight);

    SurfaceConfiguration surfaceConfig {
        .device = device,
        .format = defaultFormat,
        .width = static_cast<uint32_t>(fWidth),
        .height = static_cast<uint32_t>(fHeight),
        .presentMode = PresentMode::Fifo,
    };
    surface.Configure(&surfaceConfig);
    return surface;
}

Texture getSurfaceTexture(const Surface& surface) {
    SurfaceTexture surfaceTexture {};
    surface.GetCurrentTexture(&surfaceTexture);

    if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessOptimal) {
        std::ostringstream oss;
        oss << "SurfaceGetCurrentTexture failed with status " << surfaceTexture.status;
        spdlog::error(oss.str());
        throw std::runtime_error("WGPU Error");
    }

    return surfaceTexture.texture;
}

}  // namespace utils
