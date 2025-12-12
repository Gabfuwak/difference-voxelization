#pragma once

#include <string>
#include <opencv2/opencv.hpp>
#include "camera.hpp"
#include "insect_swarm.hpp"
#include "scene_object.hpp"
#include "renderer.hpp"
#include "detect_object.hpp"  // For CameraFrame

namespace scene {

class ObservationCamera {
public:
    ObservationCamera(Camera camera,
                      const InsectSwarmConfig& insectConfig,
                      std::shared_ptr<Mesh> insectMesh,
                      std::shared_ptr<Material> insectMaterial)
        : camera_(std::move(camera))
        , swarm_(camera_, insectConfig, insectMesh, insectMaterial)
    {}

    void update() {
        swarm_.update();
    }

    CameraFrame captureFrame(core::Renderer& renderer,
                             const std::vector<SceneObject>& sceneObjects,
                             wgpu::TextureView depthView,
                             const std::string& debugWindowName = "") {
        // Combine scene + insects
        std::vector<SceneObject> toRender = sceneObjects;
        const auto& insects = swarm_.getObjects();
        toRender.insert(toRender.end(), insects.begin(), insects.end());

        // Render and capture
        renderer.renderScene(toRender, camera_, depthView);
        cv::Mat currentFrame = renderer.captureFrame();

        // Debug display
        if (!debugWindowName.empty()) {
            cv::imshow(debugWindowName, currentFrame);
        }

        // Build CameraFrame
        CameraFrame frame{
            camera_,
            currentFrame,
            previousFrame_.empty() ? currentFrame : previousFrame_
        };

        previousFrame_ = currentFrame;
        return frame;
    }

    const Camera& getCamera() const { return camera_; }

private:
    Camera camera_;
    InsectSwarm swarm_;
    cv::Mat previousFrame_;
};

} // namespace scene
