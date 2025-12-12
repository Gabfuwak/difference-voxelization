#include "GLFW/glfw3.h"
#include "app.hpp"
#include "renderer.hpp"
#include "webgpu/webgpu_cpp.h"
#include "webgpu_utils.hpp"
#include <glm/glm.hpp>
#include "triangle.hpp"

class TriangleApp : public WgpuApp {
public:
    TriangleRenderer triangleRenderer;

    TriangleApp() {}

    void initialize() {
        WgpuApp::initialize();
        triangleRenderer = TriangleRenderer(device, getSurfaceFormat());
    };

    void start() {
        clearSurface();
        surface.Present();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            // clearSurface();
            render();
            surface.Present();
        }
    }

    void render() {
        auto commandEncoder = device.CreateCommandEncoder();
        std::vector<wgpu::RenderPassColorAttachment> colorAttachments {{
            .view = getSurfaceView(),
            .loadOp = LoadOp::Clear,
            .storeOp = StoreOp::Store,
            .clearValue = {1, 1, 1, 1},
        }};
        RenderPassDescriptor passDesc {
            .label = "triangle",
            .colorAttachmentCount = colorAttachments.size(),
            .colorAttachments = colorAttachments.data(),
        };
        auto pass = commandEncoder.BeginRenderPass(&passDesc);
        // pass.SetPipeline(pipeline);
        // pass.Draw(3);
        triangleRenderer.render(pass);

        pass.End();
        auto command = commandEncoder.Finish();
        queue.Submit(1, &command);
    }
};

int main() {
    auto app = TriangleApp();
    app.initialize();
    app.start();
    return 0;
}