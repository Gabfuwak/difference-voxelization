#include <iostream>
#include <GLFW/glfw3.h>

#include "core/context.hpp"
#include "core/renderer.hpp"
#include <opencv2/opencv.hpp>
#include "scene/transform.hpp"
#include "scene/mesh.hpp"
#include "scene/camera.hpp"

// Uniform buffer structure (must match shader)
struct Uniforms {
    float mvp[16];
};

int main() {
    // Initialize core
    core::Context ctx;
    if (!ctx.initialize()) {
        std::cerr << "Failed to initialize WebGPU context\n";
        return 1;
    }

    // Create window
    /*core::Window window(800, 600, "WebGPU Cube");
    if (!window.create()) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    window.createSurface(ctx.instance, ctx.adapter, ctx.device);
    */

    // Create renderer
    core::Renderer renderer(&ctx, 800, 600);
    renderer.createUniformBuffer(sizeof(Uniforms));
    renderer.createPipeline(SHADERS_DIR "unlit.wgsl");

    // Create depth texture
    wgpu::Texture depthTexture = renderer.createDepthTexture();
    wgpu::TextureView depthView = depthTexture.CreateView();

    // Create scene
    scene::Mesh suzanneMesh = scene::Mesh::createMesh("models/Suzanne.obj", ctx.device, ctx.queue);
    scene::Transform suzanneTransform;
    scene::Camera camera(800.0f / 600.0f);
    camera.position = {2.0f, 2.0f, 3.0f};

    
    float time = 0.0f;
    while (true) {
        time += 0.016f;
        
        suzanneTransform.rotate(0.016f * 0.5f, Eigen::Vector3f::UnitY());
        
        Eigen::Matrix4f mvp = camera.getViewProjectionMatrix() * suzanneTransform.getMatrix();
        Uniforms uniforms;
        memcpy(uniforms.mvp, mvp.data(), sizeof(float) * 16);
        renderer.updateUniformBuffer(&uniforms, sizeof(uniforms));
        
        renderer.render(suzanneMesh.vertexBuffer, suzanneMesh.indexBuffer, 
                       suzanneMesh.indexCount, depthView);
        
        cv::Mat frame = renderer.captureFrame();
        
        
        cv::imshow("WebGPU Render", frame);
        
        int key = cv::waitKey(1);
        
        if (key == 27) break;
        
        ctx.processEvents();
    }
    

    return 0;
}
