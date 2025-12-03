#pragma once

#include <iostream>
#include <string_view>
#include <webgpu/webgpu_cpp.h>

namespace core {

// Helper to convert wgpu::StringView to std::string_view for printing
inline std::string_view toSV(wgpu::StringView sv) {
    return std::string_view(sv.data, sv.length);
}

class Context {
public:
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue queue;

    bool initialize() {
        createInstance();
        if (!instance) return false;
        
        requestAdapter();
        if (!adapter) return false;
        
        requestDevice();
        if (!device) return false;
        
        queue = device.GetQueue();
        return true;
    }

    void processEvents() {
        instance.ProcessEvents();
    }

private:
    void createInstance() {
        wgpu::InstanceDescriptor descriptor{};
        instance = wgpu::CreateInstance(&descriptor);
    }

    void requestAdapter() {
        wgpu::RequestAdapterOptions options{};
        bool adapterReady = false;
        wgpu::StringView errorMessage{};

        auto callback = [this, &adapterReady, &errorMessage](
            wgpu::RequestAdapterStatus status, 
            wgpu::Adapter adapter, 
            wgpu::StringView message) {
            if (status == wgpu::RequestAdapterStatus::Success) {
                this->adapter = std::move(adapter);
            } else {
                errorMessage = message;
            }
            adapterReady = true;
        };

        instance.RequestAdapter(&options, wgpu::CallbackMode::AllowProcessEvents, callback);

        // Process events until the adapter request completes
        while (!adapterReady) {
            instance.ProcessEvents();
        }

        if (!adapter) {
            std::cerr << "Failed to get adapter: " << toSV(errorMessage) << '\n';
        }
    }

    void requestDevice() {
        wgpu::DeviceDescriptor desc{};

        desc.SetUncapturedErrorCallback([](
            const wgpu::Device& device,
            wgpu::ErrorType type,
            wgpu::StringView message
        ) {
            std::cerr << "WebGPU Error: " << static_cast<int>(type) << " - " << toSV(message) << '\n';
        });

        desc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, [](
            const wgpu::Device& device,
            wgpu::DeviceLostReason reason,
            wgpu::StringView message
        ) {
            std::cerr << "Device lost: " << toSV(message) << '\n';
        });

        bool deviceReady = false;
        wgpu::StringView errorMessage{};

        auto callback = [this, &deviceReady, &errorMessage](
            wgpu::RequestDeviceStatus status, 
            wgpu::Device device, 
            wgpu::StringView message) {
            if (status == wgpu::RequestDeviceStatus::Success) {
                this->device = std::move(device);
            } else {
                errorMessage = message;
            }
            deviceReady = true;
        };

        adapter.RequestDevice(&desc, wgpu::CallbackMode::AllowProcessEvents, callback);

        // Process everts until the device request completes
        while (!deviceReady) {
            instance.ProcessEvents();
        }

        if (!device) {
            std::cerr << "Failed to get device: " << toSV(errorMessage) << '\n';
        }
    } 

        
};

} // namespace core
