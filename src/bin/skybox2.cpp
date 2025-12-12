#include <iostream>
#include <limits>
#include <webgpu/webgpu_cpp.h>
#include "GLFW/glfw3.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/matrix.hpp"
#include "imgui.h"
#include "imgui_utils.hpp"
#include "webgpu_utils.hpp"
#include <utils.hpp>
#include "stb_image.h"
#include <vector>
#include <array>
#include <ranges>
#include "utils.hpp"
#include <camera2.hpp>
#include <image.hpp>
#include <webgpu_utils.hpp>
#include <glfw_utils.hpp>
#include "skybox.hpp"
#include "globals.hpp"

using namespace wgpu;
using namespace glm;

struct Cursor {
    double lastX = std::numeric_limits<double>::quiet_NaN();
    double lastY = std::numeric_limits<double>::quiet_NaN();
    double deltaX = std::numeric_limits<double>::quiet_NaN();
    double deltaY = std::numeric_limits<double>::quiet_NaN();
    int deltaSkipCounter = 0;
    bool dragging = false;
};

class SkyboxApp {
public:
    std::string title = "Skybox App";
    uint32_t width = 800;
    uint32_t height = 600;

    Cursor cursor;

    GLFWwindow* window;
    Instance instance;
    Adapter adapter;
    Device device;
    Queue queue;
    Surface surface;
    RenderPipeline pipeline;

    Globals globals;

    float time;

    FreeCamera camera;

    SkyboxRenderer skyboxRenderer;
    SkyboxMaterial skyboxMaterial;

    Texture depthTexture;
    TextureFormat depthFormat = TextureFormat::Depth24Plus;

    TextureFormat surfaceFormat;

    std::array<std::string, 6> skyboxFacePaths {
        "leadenhall_market/pos-x.jpg",
        "leadenhall_market/neg-x.jpg",
        "leadenhall_market/pos-y.jpg",
        "leadenhall_market/neg-y.jpg",
        "leadenhall_market/pos-z.jpg",
        "leadenhall_market/neg-z.jpg",
    };

    SkyboxApp() {}

    void initialize() {
        if (!glfwInit()) {
            exit(1);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        instance = utils::createInstance();
        adapter = utils::requestAdapterSync(instance);
        device = utils::requestDeviceSync(instance, adapter);
        queue = device.GetQueue();

        createWindow();
        createSurface();
        createDepthTexture();

        utils::imguiInitialize(window, device, surface);
        globals.initialize(device);
        skyboxMaterial.initialize(device, skyboxFacePaths);
        skyboxRenderer.initialize(device, surfaceFormat, depthFormat, globals.layout);

        createPipeline();
        createCamera();
    }

    void createDepthTexture() {
        depthTexture = utils::createDepthTexture(device, window);
    }

    void createCamera() {
        camera = FreeCamera();
        auto [width, height] = utils::getFramebufferSize(window);
        camera.aspect = 1.0f * width / height;
    }

    void updateGlobals() {
        auto viewProjection = camera.viewProjection();
        auto viewProjectionInv = glm::inverse(viewProjection);
        globals.data.viewProjection = viewProjection;
        globals.data.viewProjectionInv = viewProjectionInv,
        globals.data.position = camera.position;
        globals.updateBuffer(queue);
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        auto* app = static_cast<SkyboxApp*>(glfwGetWindowUserPointer(window));
        if (app == nullptr) {
            return;
        }
        // app->onMouseButton(button, action, mods);
    }

    void onCursorPos(double x, double y) {
        if (cursor.dragging) {
            cursor.deltaX = x - cursor.lastX;
            cursor.deltaY = y - cursor.lastY;
        } else {
            cursor.deltaX = cursor.deltaY = 0.0f;
        }
        cursor.lastX = x;
        cursor.lastY = y;

        if (cursor.dragging) {
            if (!cursor.deltaSkipCounter) {
                camera.pan(cursor.deltaX, cursor.deltaY, 0.01f);
            } else {
                cursor.deltaSkipCounter--;
            }
        }
    }

    static void cursorPosCallback(GLFWwindow* window, double x, double y) {
        auto* app = static_cast<SkyboxApp*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        app->onCursorPos(x, y);
    }

    void onScroll(double dx, double dy) {
        camera.zoom(dy, 0.1);
    }

    void onMouseButton(int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                cursor.dragging = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwGetCursorPos(window, &cursor.lastX, &cursor.lastY);
                cursor.deltaSkipCounter = 2;
            } else if (action == GLFW_RELEASE) {
                cursor.dragging = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }

    void installGlfwCallbacks() {

    }

    void start() {
        double startTime = glfwGetTime();

        time = 0;
        render();
        surface.Present();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            utils::imguiNewFrame();
            bool showDemo = true;
            ImGui::ShowDemoWindow(&showDemo);

            time = glfwGetTime() - startTime;
            render();
            utils::imguiRender(device, surface);
            surface.Present();
        }
    }

    void createPipeline() {
        auto shaderModule = utils::loadShaderModule(device, "skybox.wgsl");
        shaderModule.SetLabel("skybox");

        VertexState vertexState {
            .module = shaderModule,
        };

        auto format = utils::getSurfaceTexture(surface).GetFormat();
        std::vector<ColorTargetState> colorTargets {{
            .format = format,
        }};
        FragmentState fragmentState {
            .module = shaderModule,
            .targetCount = colorTargets.size(),
            .targets = colorTargets.data(),
        };

        std::vector<BindGroupLayout> bindGroupLayouts {
            globals.layout,
            skyboxMaterial.layout,
        };
        PipelineLayoutDescriptor layoutDesc {
            .label = "skybox",
            .bindGroupLayoutCount = bindGroupLayouts.size(),
            .bindGroupLayouts = bindGroupLayouts.data(),
        };
        auto layout = device.CreatePipelineLayout(&layoutDesc);

        RenderPipelineDescriptor pipelineDesc {
            .layout = layout,
            .vertex = vertexState,
            .fragment = &fragmentState,
        };
        pipeline = device.CreateRenderPipeline(&pipelineDesc);
    }

    void render() {
        updateGlobals();

        CommandEncoderDescriptor commandEncoderDesc {.label = "skybox app"};
        auto commandEncoder = device.CreateCommandEncoder(&commandEncoderDesc);

        auto surfaceTexture = utils::getSurfaceTexture(surface);
        std::vector<RenderPassColorAttachment> colorAttachments {{
            .view = surfaceTexture.CreateView(),
            .loadOp = LoadOp::Clear,
            .storeOp = StoreOp::Store,
            .clearValue = {1, 1, 1, 0},
        }};

        RenderPassDepthStencilAttachment depthStencilAttachment {
            .view = depthTexture.CreateView(),
            .depthLoadOp = LoadOp::Clear,
            .depthStoreOp = StoreOp::Store,
            .depthClearValue = 1.0f,
        };

        RenderPassDescriptor passDesc {
            .label = "skybox app",
            .colorAttachmentCount = colorAttachments.size(),
            .colorAttachments = colorAttachments.data(),
            .depthStencilAttachment = &depthStencilAttachment,
        };
        auto pass = commandEncoder.BeginRenderPass(&passDesc);

        skyboxRenderer.render(pass, globals.bindGroup, skyboxMaterial.bindGroup);

        // pass.SetPipeline(pipeline);
        // pass.SetBindGroup(0, globals.bindGroup);
        // pass.SetBindGroup(1, skyboxMaterial.bindGroup);
        // pass.Draw(3);
        pass.End();

        std::vector<CommandBuffer> commands {{commandEncoder.Finish()}};
        queue.Submit(commands.size(), commands.data());
    }

    void createWindow() {
        window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, SkyboxApp::mouseButtonCallback);
        glfwSetCursorPosCallback(window, SkyboxApp::cursorPosCallback);
        glfwSetScrollCallback(window, SkyboxApp::scrollCallback);
    }

    static void scrollCallback(GLFWwindow* window, double dx, double dy) {
        auto app = static_cast<SkyboxApp*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        app->onScroll(dx, dy);
    }

    void createSurface() {
        surface = utils::createSurfaceWithPreferredFormat(device, window);
        surfaceFormat = utils::getSurfaceTexture(surface).GetFormat();
    }
};

int main() {
    auto app = SkyboxApp();
    app.initialize();
    app.start();

    return 0;
}
