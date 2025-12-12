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
    uint32_t surfaceWidth;
    uint32_t surfaceHeight;

    int activeCamera = 0;

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
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(handle, &fbWidth, &fbHeight);
        surfaceWidth = static_cast<uint32_t>(fbWidth);
        surfaceHeight = static_cast<uint32_t>(fbHeight);

        wgpu::SurfaceConfiguration config {
            .device = device,
            .format = format,
            .width = surfaceWidth,
            .height = surfaceHeight,
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
        glfwSetWindowUserPointer(handle, this);
        glfwSetKeyCallback(handle, keyCallback);
    }

private:
    static void keyCallback(GLFWwindow* window, int key, int, int action, int) {
        if (action != GLFW_PRESS) {
            return;
        }

        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!self) {
            return;
        }

        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            self->activeCamera = key - GLFW_KEY_0;
        }

        if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
            self->activeCamera = key - GLFW_KEY_KP_0;
        }
    }
};

} // namespace core
