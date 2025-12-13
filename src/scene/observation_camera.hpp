#pragma once

#include <string>
#include "camera.hpp"
#include "insect_swarm.hpp"
#include "scene_object.hpp"

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

    const Camera& getCamera() const { return camera_; }
    
    const std::vector<SceneObject>& getInsects() const { 
        return swarm_.getObjects(); 
    }

private:
    Camera camera_;
    InsectSwarm swarm_;
};

} // namespace scene
