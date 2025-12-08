#pragma once

#include <iostream>
#include <string>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>
#include <webgpu/webgpu_glfw.h>

namespace core {

class Window {
public:
    GLFWwindow* handle = nullptr;
    uint32_t width;
    uint32_t height;
    std::string title;

    wgpu::Surface surface;
    wgpu::TextureFormat format;

    Window(uint32_t width, uint32_t height, const std::string& title)
        : width(width), height(height), title(title) {}

    bool create() {
        if (!glfwInit()) {
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        setCallbacks();

        return handle != nullptr;
    }

    void createSurface(wgpu::Instance instance, wgpu::Adapter adapter, wgpu::Device device) {
        surface = wgpu::glfw::CreateSurfaceForWindow(instance, handle);

        wgpu::SurfaceCapabilities capabilities;
        surface.GetCapabilities(adapter, &capabilities);
        format = capabilities.formats[0];
        std::cout << "Using format: " << format << '\n';

        configureSurface(device);
    }

    void configureSurface(wgpu::Device device) {
        wgpu::SurfaceConfiguration config{
            .device = device,
            .format = format,
            .width = width,
            .height = height,
            .presentMode = wgpu::PresentMode::Fifo,
        };
        surface.Configure(&config);
    }

    bool shouldClose() const {
        return glfwWindowShouldClose(handle);
    }

    void pollEvents() {
        glfwPollEvents();
    }

    void present() {
        surface.Present();
    }

    wgpu::TextureView getCurrentTextureView() {
        wgpu::SurfaceTexture surfaceTexture;
        surface.GetCurrentTexture(&surfaceTexture);
        return surfaceTexture.texture.CreateView();
    }

    wgpu::Texture getCurrentTexture() {
        wgpu::SurfaceTexture surfaceTexture;
        surface.GetCurrentTexture(&surfaceTexture);
        return surfaceTexture.texture;
    }

    ~Window() {
        if (handle) {
            glfwDestroyWindow(handle);
            glfwTerminate();
        }
    }

    void setCallbacks() {
        auto closeShortcutsKeyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (action != GLFW_PRESS) {
                return;
            }
            if (key == GLFW_KEY_W && (mods & GLFW_MOD_SUPER)) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL)) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        };
        glfwSetKeyCallback(handle, closeShortcutsKeyCallback);
    }
};

} // namespace core
