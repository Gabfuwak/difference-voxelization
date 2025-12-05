#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>

#include "core/context.hpp"
#include "core/renderer.hpp"
#include "scene/scene_object.hpp"
#include <opencv2/opencv.hpp>

int main() {
    core::Context ctx;
    if (!ctx.initialize()) {
        std::cerr << "Failed to initialize WebGPU context\n";
        return 1;
    }

    core::Renderer renderer(&ctx, 800, 600);
    renderer.createUniformBuffer(sizeof(float) * 16);
    renderer.createPipeline(SHADERS_DIR "unlit.wgsl");

    wgpu::Texture depthTexture = renderer.createDepthTexture();
    wgpu::TextureView depthView = depthTexture.CreateView();

    // Create meshes (shared)
    auto suzanneMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createMesh("models/Suzanne.obj", ctx.device, ctx.queue));
    auto terrainMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createGridPlane(ctx.device, ctx.queue));

    // Create scene objects
    std::vector<scene::SceneObject> objects = {
        {terrainMesh, scene::Transform()},
        {suzanneMesh, scene::Transform()},
    };

    std::vector<scene::Camera> cameras;

    scene::Camera cam1(800.0f / 600.0f);
    cam1.position = {2.0f, 2.0f, 3.0f};
    cameras.push_back(cam1);

    scene::Camera cam2(800.0f / 600.0f);
    cam2.position = {-2.0f, 2.0f, 3.0f};
    cameras.push_back(cam2);

    while (true) {
        // Rotate suzanne (index 1)
        objects[1].transform.rotate(0.016f * 0.5f, Eigen::Vector3f::UnitY());

        for (size_t i = 0; i < cameras.size(); ++i) {
            renderer.renderScene(objects, cameras[i], depthView);
            cv::Mat frame = renderer.captureFrame();
            cv::imshow("Camera " + std::to_string(i), frame);
        }

        if (cv::waitKey(1) == 27) break;

        ctx.processEvents();
    }

    return 0;
}
