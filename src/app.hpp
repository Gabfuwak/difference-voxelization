#include "imgui_impl_glfw.h"
#include "webgpu_utils.hpp"
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

using namespace wgpu;

class WgpuApp {
public:
    const char *title = "WebGPU App";
    GLFWwindow* window;
    int width = 800;
    int height = 600;

    Instance instance;
    Adapter adapter;
    Device device;
    Queue queue;
    Surface surface;
    Texture depthTexture;
    RenderPipeline pipeline;

    void initialize() {
        // Initialize GLFW
        if (!glfwInit()) exit(1);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Initialize WebGPU
        instance = utils::createInstance();
        adapter = utils::requestAdapterSync(instance);
        device = utils::requestDeviceSync(instance, adapter);
        queue = device.GetQueue();

        createWindow();
        createSurface();
        createDepthTexture();
    }

    void start() {
        clearSurface();
        surface.Present();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            clearSurface();
            surface.Present();
        }
    }

    void createWindow() {
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
    }

    void createSurface() {
        surface = utils::createSurfaceWithPreferredFormat(device, window);
    }

    void createDepthTexture() {
        depthTexture = utils::createDepthTexture(device, window);
    }

    void clearSurface() {
        auto commandEncoder = device.CreateCommandEncoder();

        std::vector<RenderPassColorAttachment> colorAttachments {{
            .view = utils::getSurfaceTexture(surface).CreateView(),
            .loadOp = LoadOp::Clear,
            .storeOp = StoreOp::Store,
            .clearValue = {1, 1, 1, 1},
        }};

        RenderPassDescriptor passDesc {
            .colorAttachmentCount = colorAttachments.size(),
            .colorAttachments = colorAttachments.data(),
        };
        auto pass = commandEncoder.BeginRenderPass(&passDesc);
        pass.End();

        auto command = commandEncoder.Finish();
        queue.Submit(1, &command);
    }

    wgpu::TextureFormat getSurfaceFormat() {
        return getSurfaceTexture().GetFormat();
    }

    wgpu::TextureFormat getDepthFormat() {
        return depthTexture.GetFormat();
    }

    wgpu::Texture getSurfaceTexture() {
        return utils::getSurfaceTexture(surface);
    }

    wgpu::TextureView getSurfaceView() {
        return getSurfaceTexture().CreateView();
    }
};
