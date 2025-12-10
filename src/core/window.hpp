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

    std::vector<GLFWkeyfun> keyCallbacks;
    int activeView = 0;

    Window(uint32_t width, uint32_t height, const std::string& title)
        : width(width), height(height), title(title) {}

    bool create() {
        if (!glfwInit()) {
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(handle, this);
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

    void addKeyCallback(GLFWkeyfun callback) {
        keyCallbacks.push_back(callback);
        glfwSetKeyCallback(handle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            auto windowPtr = static_cast<Window*>(glfwGetWindowUserPointer(window));
            for (auto cb : windowPtr->keyCallbacks) {
                cb(window, key, scancode, action, mods);
            }
        });
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
        auto selectCameraKeyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            auto windowPtr = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (action != GLFW_PRESS) return;
            if (key == GLFW_KEY_0) { // debug view
                std::cout << "Debug view" << '\n';
                windowPtr->activeView = 0;
            }
            if (key == GLFW_KEY_1) { // camera 1
                std::cout << "Select camera 1" << '\n';
                windowPtr->activeView = 1;

            }
            if (key == GLFW_KEY_2) { // camera 2
                std::cout << "Select camera 2" << '\n';
                windowPtr->activeView = 2;
            }
            if (key == GLFW_KEY_3) { // camera 3
                std::cout << "Select camera 3" << '\n';
                windowPtr->activeView = 3;
            }
        };
        addKeyCallback(closeShortcutsKeyCallback);
        addKeyCallback(selectCameraKeyCallback);
    }
};

} // namespace core
