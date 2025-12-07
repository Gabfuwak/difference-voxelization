#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>

#include "core/context.hpp"
#include "core/renderer.hpp"
#include "scene/scene_object.hpp"
#include "vision/detect_object.hpp"
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



    // Terrain - 500m x 500m
    auto terrainMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createGridPlane(ctx.device, ctx.queue, 500.0f, 50));

    // Meshes
    auto houseMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createMesh("models/house.obj", ctx.device, ctx.queue));
    auto treeMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createMesh("models/MapleTree.obj", ctx.device, ctx.queue));

    std::vector<scene::SceneObject> objects;

    // Terrain at origin
    objects.push_back({terrainMesh, scene::Transform()});


    // Create drone mesh (cube scaled to 50cm)
    auto droneMesh = std::make_shared<scene::Mesh>(
        scene::Mesh::createCube(ctx.device, ctx.queue));

    scene::Transform droneTransform;
        droneTransform.scale = Eigen::Vector3f(0.5f, 0.5f, 0.5f);
        objects.push_back({droneMesh, droneTransform});

    size_t droneIndex = objects.size() - 1;  // remember index for animation

    // House - left side
    scene::Transform houseTransform;
    houseTransform.position = Eigen::Vector3f(-150.0f, 0.0f, 0.0f);
    objects.push_back({houseMesh, houseTransform});

    // Tree helper lambda
    auto addTree = [&](float x, float z) {
        scene::Transform t;
        t.position = Eigen::Vector3f(x, 0.0f, z);
        //t.scale = Eigen::Vector3f(10.0f, 10.0f, 10.0f);
        objects.push_back({treeMesh, t});
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
    
    std::vector<scene::Camera> cameras;

    auto makeCamera = [](Eigen::Vector3f pos, Eigen::Vector3f target) {
        scene::Camera cam(800.0f / 600.0f);
        cam.position = pos;
        cam.target = target;
        cam.farPlane = 1000.0f;
        return cam;
    };

    // Height: 2m (human eye level), looking up so 80% sky
    // Positions at edges, ~200m out from center

    // Bottom camera (south edge, looking north + up)
    cameras.push_back(makeCamera(
        {0.0f, 2.0f, 200.0f},
        {0.0f, 80.0f, -100.0f}
    ));

    // Left camera (west edge, looking east + up)
    cameras.push_back(makeCamera(
        {-200.0f, 2.0f, 0.0f},
        {100.0f, 80.0f, 0.0f}
    ));

    // Top camera (north edge, looking south + up)
    cameras.push_back(makeCamera(
        {0.0f, 2.0f, -200.0f},
        {0.0f, 80.0f, 100.0f}
    ));

    // Right camera (east edge, looking west + up)
    cameras.push_back(makeCamera(
        {200.0f, 2.0f, 0.0f},
        {-100.0f, 80.0f, 0.0f}
    ));
    float time = 0.0f;

    std::vector<cv::Mat> previous_frames;  // Same index as cameras
    previous_frames.resize(cameras.size());

    while (true) {
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

        // Build frames with previous frame data
        std::vector<CameraFrame> frames;
        for (size_t i = 0; i < cameras.size(); ++i) {
            renderer.renderScene(objects, cameras[i], depthView);
            cv::Mat curr_frame = renderer.captureFrame();
            cv::imshow("Camera " + std::to_string(i), curr_frame);
            
            frames.push_back({
                cameras[i], 
                curr_frame, // shallow copy
                previous_frames[i].empty() ? curr_frame : previous_frames[i]  // use current on first frame
            });
            
            previous_frames[i] = curr_frame;
        }
        
        Voxel target_zone = Voxel{{0., 0., 0.}, // center
                                   250.};       // half size

        float min_voxel_size = 0.2; // cube is 0.5 large
        size_t min_ray_threshold = 3; // at least 3 rays for detection
        auto detections = detect_objects(target_zone, frames, min_voxel_size, min_ray_threshold);


        std::cout << "Frame " << time << " - Detections: " << detections.size() << std::endl;
        if (!detections.empty()) {
            Eigen::Vector3d centroid(0, 0, 0);
            for (const auto& det : detections) {
                centroid += det.center;
            }
            centroid /= detections.size();
            
            std::cout << "  Detection centroid: ("
                      << centroid.x() << ", "
                      << centroid.y() << ", "
                      << centroid.z() << ")" << std::endl;
        }

        std::cout << "Actual drone position: (" 
                  << objects[droneIndex].transform.position.x() << ", "
                  << objects[droneIndex].transform.position.y() << ", "
                  << objects[droneIndex].transform.position.z() << ")" << std::endl;
        std::cout << "---" << std::endl;

        if (cv::waitKey(1) == 27) break;
        ctx.processEvents();
    }

    return 0;
}
