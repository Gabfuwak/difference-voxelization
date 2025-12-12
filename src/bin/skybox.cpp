#include "GLFW/glfw3.h"
#include "app.hpp"
#include "renderer.hpp"
#include "webgpu/webgpu_cpp.h"
#include "webgpu_utils.hpp"
#include <glm/glm.hpp>
#include <image.hpp>
#include "skybox.hpp"

class SkyboxApp : public WgpuApp {
public:
    SkyboxRenderer skyboxRenderer;
    BindGroupLayout globalsLayout;

    BindGroup globalsBindGroup;
    BindGroup skyboxMaterialBindGroup;

    SkyboxApp() {}

    void initialize() {
        WgpuApp::initialize();
        createGlobalsLayout();
        // skyboxRenderer = SkyboxRenderer(device, getSurfaceFormat(), getDepthFormat(), globalsLayout);
    };

    void start() {
        render();
        surface.Present();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            render();
            surface.Present();
        }
    }

    void createGlobalsLayout() {
        std::vector<BindGroupLayoutEntry> entries {{
            .binding = 0,
            .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
            .buffer = {.type = BufferBindingType::Uniform},
        }};
        BindGroupLayoutDescriptor layoutDesc {
            .label = "globals",
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        globalsLayout = device.CreateBindGroupLayout(&layoutDesc);
    }

    void render() {
        clearSurface();
    }
};

int main() {
    auto app = SkyboxApp();
    app.initialize();
    app.start();
    return 0;
}