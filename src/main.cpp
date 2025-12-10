#include <iostream>
#include <sstream>
#include <vector>
#include <GLFW/glfw3.h>

#include "core/context.hpp"
#include "core/renderer.hpp"
#include "core/window.hpp"
#include "scene/scene_object.hpp"
#include "scene/observation_camera.hpp"
#include "vision/detect_object.hpp"
#include "webgpu/webgpu_cpp.h"
#include <opencv2/opencv.hpp>

int main() {
    core::Context ctx;
    if (!ctx.initialize()) {
        std::cerr << "Failed to initialize WebGPU context\n";
        return 1;
    }


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

    core::Renderer renderer(&ctx, 800, 600);
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
                        "models/maple_leaf_Mask.png")); // Add mask!
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
        .movementSpeed = 0.1f
    };

    auto makeObserver = [&](Eigen::Vector3f pos, Eigen::Vector3f target) {
        scene::Camera cam(800.0f / 600.0f);
        cam.position = pos;
        cam.target = target;
        cam.farPlane = 1000.0f;
        return scene::ObservationCamera(cam, insectConfig, insectMesh, defaultMaterial);
    };

    std::vector<scene::ObservationCamera> observers;
    observers.push_back(makeObserver({0.0f, 2.0f, 200.0f}, {0.0f, 80.0f, -100.0f}));
    addTree(0.0f, 180.0f); // in front of camera for occlusion
    observers.push_back(makeObserver({-200.0f, 2.0f, 0.0f}, {100.0f, 80.0f, 0.0f}));
    observers.push_back(makeObserver({0.0f, 2.0f, -200.0f}, {0.0f, 80.0f, 100.0f}));
    observers.push_back(makeObserver({200.0f, 2.0f, 0.0f}, {-100.0f, 80.0f, 0.0f}));




    float time = 0.0f;
    double avg_detection_time = 0.0;
    double total_error = 0.0;
    int frame_count = 0;



    core::Window debugWindow(800, 600, "Debug");
    debugWindow.create();
    debugWindow.createSurface(ctx.instance, ctx.adapter, ctx.device);

    while (!debugWindow.shouldClose()) {
    //while (time <= 0.02) {
        glfwPollEvents();

        time += 0.016f;

        // Drone flies in circle: 50m radius, 30m height
        float radius = 50.0f;
        float height = 30.0f;
        float speed = 2.f;

        objects[droneIndex].transform.position = Eigen::Vector3f(
            radius * std::cos(time * speed),
            height,
            radius * std::sin(time * speed)
        );


        objects[droneIndex].transform.setEulerAngles(0.0f, -time * speed, 0.0f);



        for (auto& observer : observers) {
            observer.update();
        }

        std::vector<CameraFrame> frames;
        frames.reserve(observers.size());
        for (size_t i = 0; i < observers.size(); ++i) {
            frames.push_back(observers[i].captureFrame(
                renderer, objects, depthView,
                "Camera " + std::to_string(i)
            ));
        }
        // renderer.renderScene(objects, cameras[0], depthView, debugWindow.getCurrentTextureView());
        // debugWindow.present();

        Voxel target_zone = Voxel{{0.f, 0.f, 0.f}, // center
                                   250.f};       // half size

        float min_voxel_size = 0.2; // cube is 0.5 large
        size_t min_ray_threshold = 3; // at least 3 rays for detection


        auto start = std::chrono::high_resolution_clock::now();
        auto detections = detect_objects(target_zone, frames, min_voxel_size, min_ray_threshold);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        frame_count++;
        avg_detection_time += (duration.count() - avg_detection_time) / frame_count;

        std::ostringstream oss;

        oss << "Detection time: " << duration.count() << " µs (avg: "
                  << avg_detection_time << " µs)" << std::endl;


        oss << "Frame " << time << " - Detections: " << detections.size() << std::endl;
        if (!detections.empty()) {
            Eigen::Vector3f centroid(0, 0, 0);
            for (const auto& det : detections) {
                centroid += det.center;
            }
            centroid /= detections.size();

            oss << "  Detection centroid: ("
                      << centroid.x() << ", "
                      << centroid.y() << ", "
                      << centroid.z() << ")" << std::endl;

            auto error = (centroid - objects[droneIndex].transform.position).norm();
            total_error += error;
            oss << "Error: " << error << "(avg:"<< total_error / frame_count<<" )" << std::endl;



            oss << "Actual drone position:("
                  << objects[droneIndex].transform.position.x() << ", "
                  << objects[droneIndex].transform.position.y() << ", "
                  << objects[droneIndex].transform.position.z() << ")" << std::endl;
        }




        oss << "---" << std::endl;

        std::cout << oss.str();

        if (cv::waitKey(1) == 27) break;

        // wgpu::CommandEncoderDescriptor commandEncoderDesc {.label = "copy"};
        // auto commandEncoder = ctx.device.CreateCommandEncoder(&commandEncoderDesc);
        // wgpu::TexelCopyTextureInfo source {.texture = renderer.targetTexture};
        // wgpu::TexelCopyTextureInfo destination {.texture = debugWindow.getCurrentTexture()};
        // wgpu::Extent3D copySize {800, 600};
        // commandEncoder.CopyTextureToTexture(&source, &destination, &copySize);
        // std::array commands {commandEncoder.Finish()};
        // ctx.queue.Submit(commands.size(), commands.data());

        ctx.processEvents();
    }

    return 0;
}
