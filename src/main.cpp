#include <iostream>
#include <sstream>
#include <vector>
#include <GLFW/glfw3.h>

#include "core/context.hpp"
#include "core/renderer.hpp"
#include "core/multi_camera_capture.hpp"
#include "core/window.hpp"
#include "scene/scene_object.hpp"
#include "scene/observation_camera.hpp"
#include "vision/detect_object.hpp"
#include "webgpu/webgpu.h"
#include "webgpu/webgpu_cpp.h"
#include <opencv2/opencv.hpp>

#include "utils.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

int main() {
    core::Context ctx;
    if (!ctx.initialize()) {
        std::cerr << "Failed to initialize WebGPU context\n";
        return 1;
    }

    core::Window debugWindow(800, 600, "Debug");
    debugWindow.create();
    debugWindow.createSurface(ctx.instance, ctx.adapter, ctx.device);

    // Create dummy mask texture (1x1 white)
    wgpu::TextureDescriptor dummyMaskDesc{};
    dummyMaskDesc.size = {1, 1, 1};
    dummyMaskDesc.format = wgpu::TextureFormat::R8Unorm;
    dummyMaskDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    wgpu::Texture dummyMaskTexture = ctx.device.CreateTexture(&dummyMaskDesc);

    uint8_t whitePixel = 255;
    wgpu::TexelCopyBufferLayout dummyLayout{};
    dummyLayout.bytesPerRow = 1;
    wgpu::TexelCopyTextureInfo destination{};
    destination.texture = dummyMaskTexture;
    wgpu::Extent3D writeSize{1, 1, 1};
    ctx.queue.WriteTexture(&destination, &whitePixel, 1, &dummyLayout, &writeSize);
    wgpu::TextureView dummyMaskView = dummyMaskTexture.CreateView();

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(debugWindow.handle, &fbWidth, &fbHeight);
    auto surfaceWidth = static_cast<uint32_t>(fbWidth);
    auto surfaceHeight = static_cast<uint32_t>(fbHeight);

    core::Renderer renderer(&ctx, surfaceWidth, surfaceHeight);
    renderer.createUniformBuffer(sizeof(float) * 16);
    renderer.createPipeline(SHADERS_DIR "unlit.wgsl");

    wgpu::Texture depthTexture = renderer.createDepthTexture();
    wgpu::TextureView depthView = depthTexture.CreateView();

    auto defaultMaterial = std::make_shared<Material>(
        Material::createUntextured(ctx.device, ctx.queue));
    defaultMaterial->createBindGroup(ctx.device, renderer.bindGroupLayout,
                                    renderer.uniformBuffer, dummyMaskView);


    // Terrain - 500m x 500m
    auto terrainMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createGridPlane(ctx.device, ctx.queue, 500.0f, 50));

    // Meshes
    auto houseMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createMesh("models/house.obj", ctx.device, ctx.queue));

    // Tree stem (bark)
    auto treeStemMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createMesh("models/MapleTreeStem.obj", ctx.device, ctx.queue));
    auto barkMaterial = std::make_shared<Material>(
        Material::create(ctx.device, ctx.queue, "models/maple_bark.png"));
    barkMaterial->createBindGroup(ctx.device, renderer.bindGroupLayout,
                             renderer.uniformBuffer, dummyMaskView);

    // Tree leaves
    auto treeLeavesMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createMesh("models/MapleTreeLeaves.obj", ctx.device, ctx.queue));

    auto leafMaterial = std::make_shared<Material>(
    Material::create(ctx.device, ctx.queue,
                        "models/maple_leaf.png",
                        "models/maple_leaf_Mask.png"));
    leafMaterial->createBindGroup(ctx.device, renderer.bindGroupLayout,
                                 renderer.uniformBuffer, dummyMaskView);

    std::vector<scene::SceneObject> objects;

    // Terrain at origin
    objects.push_back({terrainMesh, scene::Transform(), defaultMaterial});


    // Create drone mesh (cube scaled to 50cm)
    auto droneMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createCube(ctx.device, ctx.queue));

    scene::Transform droneTransform;
        droneTransform.scale = Eigen::Vector3f(0.5f, 0.5f, 0.5f);
        objects.push_back({droneMesh, droneTransform, defaultMaterial});

    size_t droneIndex = objects.size() - 1;  // remember index for animation

    // House - left side
    scene::Transform houseTransform;
    houseTransform.position = Eigen::Vector3f(-150.0f, 0.0f, 0.0f);
    objects.push_back({houseMesh, houseTransform, defaultMaterial});

    // Tree helper lambda
    auto addTree = [&](float x, float z) {
        scene::Transform t;
        t.position = Eigen::Vector3f(x, 0.0f, z);

        objects.push_back({treeStemMesh, t, barkMaterial});   // stem
        objects.push_back({treeLeavesMesh, t, leafMaterial}); // leaves
    };

    // Top-left cluster (behind house)
    addTree(-180.0f, -80.0f);
    addTree(-160.0f, -100.0f);
    addTree(-140.0f, -70.0f);
    addTree(-190.0f, -110.0f);

    // Top-right trees
    addTree(100.0f, -90.0f);
    addTree(130.0f, -70.0f);

    // Bottom-right cluster
    addTree(120.0f, 80.0f);
    addTree(140.0f, 100.0f);
    addTree(100.0f, 110.0f);
    addTree(160.0f, 90.0f);
    addTree(130.0f, 130.0f);

    // Height: 2m (human eye level), looking up so 80% sky
    // Positions at edges, ~200m out from center

    auto insectMesh = std::make_shared<scene::Mesh>(scene::Mesh::createCube(ctx.device, ctx.queue));

    scene::InsectSwarmConfig insectConfig{
        .count = 10,
        .distance = 2.0f,
        .spread = 3.0f,
        .zoneHalfSize = 2.0f,
        .movementSpeed = 0.1f,
        .insectSize = 0.01f
    };

    auto makeObserver = [&](Eigen::Vector3f pos, Eigen::Vector3f target) {
        scene::Camera cam(800.0f / 600.0f);
        cam.position = pos;
        cam.target = target;
        cam.farPlane = 1000.0f;
        return scene::ObservationCamera(cam, insectConfig, insectMesh, defaultMaterial);
    };

    std::vector<scene::ObservationCamera> observers;

    observers.push_back(makeObserver({0.0f, 2.0f, 200.0f}, {0.0f, 30.0f, 0.0f}));      // Center, eye level
    observers.push_back(makeObserver({-60.0f, 2.0f, 191.0f}, {0.0f, 30.0f, 0.0f}));   // Left side
    observers.push_back(makeObserver({60.0f, 2.0f, 191.0f}, {0.0f, 30.0f, 0.0f}));    // Right side
    observers.push_back(makeObserver({0.0f, 15.0f, 199.0f}, {0.0f, 30.0f, 0.0f}));    // Higher up
    observers.push_back(makeObserver({-30.0f, 8.0f, 197.0f}, {0.0f, 30.0f, 0.0f}));   // Off-center

    // Optional: Add trees for some occlusion on this side
    addTree(0.0f, 180.0f);
    addTree(-40.0f, 175.0f);
    addTree(40.0f, 175.0f);

    core::MultiCameraCapture capture(&ctx, observers.size(), 800, 600);
    std::vector<cv::Mat> previousFrames(observers.size());




    float curr_simulation_time = 0.0f;
    double avg_detection_time = 0.0;
    double total_error = 0.0;
    int frame_count = 0;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOther(debugWindow.handle, true);
    ImGui_ImplWGPU_InitInfo info;
    info.Device = ctx.device.Get();
    info.NumFramesInFlight = 3;
    info.RenderTargetFormat = static_cast<WGPUTextureFormat>(debugWindow.format);
    info.DepthStencilFormat = static_cast<WGPUTextureFormat>(wgpu::TextureFormat::Depth24Plus);
    ImGui_ImplWGPU_Init(&info);

    
    while (!debugWindow.shouldClose()) {
    //while(time <= 0.02) {
        glfwPollEvents();

        double curr_real_time = glfwGetTime();
        frame_count++;

        curr_simulation_time += 0.016f; 

        // Drone flies in circle: 50m radius, 30m height
        float radius = 50.0f;
        float height = 30.0f;
        float speed = 2.f;

        objects[droneIndex].transform.position = Eigen::Vector3f(
            radius * std::cos(curr_simulation_time * speed),
            height,
            radius * std::sin(curr_simulation_time * speed)
        );
        objects[droneIndex].transform.setEulerAngles(0.0f, -curr_simulation_time * speed, 0.0f);

        for (auto& observer : observers) {
            observer.update();
        }

        // Collect cameras
        std::vector<scene::Camera> cameras;
        cameras.reserve(observers.size());
        for (auto& observer : observers) {
            cameras.push_back(observer.getCamera());
        }

        // Gather all objects including insects from all observers
        std::vector<scene::SceneObject> allObjects = objects;
        for (auto& observer : observers) {
            const auto& insects = observer.getInsects();
            allObjects.insert(allObjects.end(), insects.begin(), insects.end());
        }

        // Render all frames in parallel
        capture.renderAll(cameras, allObjects, renderer);
        capture.copyAll();
        capture.sync();
        std::vector<cv::Mat> currentFrames = capture.readAll();

        // Build CameraFrame array
        std::vector<CameraFrame> frames;
        frames.reserve(observers.size());
        for (size_t i = 0; i < observers.size(); ++i) {
            frames.push_back({
                cameras[i],
                currentFrames[i],
                previousFrames[i].empty() ? currentFrames[i] : previousFrames[i]
            });
            previousFrames[i] = currentFrames[i].clone();
        }

        // Detection
        Voxel target_zone = Voxel{{0.f, 0.f, 0.f}, 250.f};
        float min_voxel_size = 0.1f;
        size_t min_ray_threshold = 3;

        auto start = std::chrono::high_resolution_clock::now();
        auto detections = detect_objects(target_zone, frames, min_voxel_size, min_ray_threshold, 8);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        avg_detection_time += (duration.count() - avg_detection_time) / frame_count;

        // Compute centroid (even if empty, for safe debug rendering)
        Eigen::Vector3f centroid(0, 0, 0);
        if (!detections.empty()) {
            for (const auto& det : detections) {
                centroid += det.center;
            }
            centroid /= detections.size();

            auto error = (centroid - objects[droneIndex].transform.position).norm();
            total_error += error;
        }

        // Debug window rendering (always runs)
        auto surfaceTextureView = debugWindow.getCurrentTextureView();
        
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplWGPU_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Stats");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Detection time: %.2f ms", avg_detection_time / 1000.0);
        ImGui::Text("Detections: %zu", detections.size());
        ImGui::Text("Error: %.3f m", (centroid - objects[droneIndex].transform.position).norm());
        ImGui::End();

        if (debugWindow.activeCamera > 0 && debugWindow.activeCamera <= static_cast<int>(observers.size())) {
            auto activeCamera = observers[debugWindow.activeCamera - 1].getCamera();
            auto viewProjection = activeCamera.getViewProjectionMatrix();

            // Draw ground truth (red)
            auto drawMarker = [&](const Eigen::Vector3f& worldPos, ImU32 color) {
                auto clipPos = viewProjection * Eigen::Vector4f(worldPos.x(), worldPos.y(), worldPos.z(), 1);
                if (clipPos.w() <= 0.0f) return;
                
                auto clipPosXy = Eigen::Vector2f(clipPos.x(), clipPos.y()) / clipPos.w();
                if (clipPosXy.x() < -1.0f || clipPosXy.x() > 1.0f ||
                    clipPosXy.y() < -1.0f || clipPosXy.y() > 1.0f) return;

                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                const ImVec2 screenPos {
                    viewport->Pos.x + (clipPosXy.x() * 0.5f + 0.5f) * viewport->Size.x,
                    viewport->Pos.y + (-clipPosXy.y() * 0.5f + 0.5f) * viewport->Size.y
                };
                constexpr float halfExtent = 10.0f;
                ImGui::GetForegroundDrawList()->AddRect(
                    ImVec2(screenPos.x - halfExtent, screenPos.y - halfExtent),
                    ImVec2(screenPos.x + halfExtent, screenPos.y + halfExtent),
                    color);
            };

            drawMarker(objects[droneIndex].transform.position, IM_COL32(255, 0, 0, 255));  // Red: ground truth
            if (!detections.empty()) {
                drawMarker(centroid, IM_COL32(0, 0, 255, 255));  // Blue: detection
            }

            renderer.renderScene(allObjects, activeCamera, depthView, surfaceTextureView, true);
        } else {
            renderer.renderImgui(depthView, surfaceTextureView, true);  // clear=true since no scene rendered
        }

        debugWindow.present();
        ctx.processEvents();
    }

    return 0;
}
