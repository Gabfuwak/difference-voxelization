#include <iostream>
#include <GLFW/glfw3.h>

#include "core/context.hpp"
#include "core/window.hpp"
#include "core/renderer.hpp"
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
    core::Window window(800, 600, "WebGPU Cube");
    if (!window.create()) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    window.createSurface(ctx.instance, ctx.adapter, ctx.device);

    // Create renderer
    core::Renderer renderer(&ctx, &window);
    renderer.createUniformBuffer(sizeof(Uniforms));
    renderer.createPipeline(SHADERS_DIR "unlit.wgsl");

    // Create depth texture
    wgpu::Texture depthTexture = renderer.createDepthTexture();
    wgpu::TextureView depthView = depthTexture.CreateView();

    // Create scene
    scene::Mesh cubeMesh = scene::Mesh::createCube(ctx.device, ctx.queue);
    scene::Transform cubeTransform;
    scene::Camera camera(800.0f / 600.0f);
    camera.position = {2.0f, 2.0f, 3.0f};

    // Main loop
    float lastTime = glfwGetTime();
    
    while (!window.shouldClose()) {
        window.pollEvents();

        // Calculate delta time
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Rotate the cube
        cubeTransform.rotate(deltaTime * 0.5f, Eigen::Vector3f::UnitY());
        cubeTransform.rotate(deltaTime * 0.3f, Eigen::Vector3f::UnitX());

        // Calculate MVP matrix
        Eigen::Matrix4f model = cubeTransform.getMatrix();
        Eigen::Matrix4f viewProj = camera.getViewProjectionMatrix();
        Eigen::Matrix4f mvp = viewProj * model;

        // Update uniform buffer (Eigen stores in column-major, which matches WGSL)
        Uniforms uniforms;
        memcpy(uniforms.mvp, mvp.data(), sizeof(float) * 16);
        renderer.updateUniformBuffer(&uniforms, sizeof(uniforms));

        // Render
        renderer.render(cubeMesh.vertexBuffer, cubeMesh.indexBuffer, 
                       cubeMesh.indexCount, depthView);

        window.present();
        ctx.processEvents();
    }

    return 0;
}
