#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include <memory>
#include "mesh.hpp"
#include "material.hpp"
#include "scene_object.hpp"
#include "camera.hpp"

namespace scene {

struct InsectSwarmConfig {
    int count = 50;
    float distance = 3.0f;
    float spread = 0.3f;
    float zoneHalfSize = 2.0f;
    float movementSpeed = 0.5f;
};

class InsectSwarm {
public:
    InsectSwarm(const Camera& camera,
                const InsectSwarmConfig& config,
                std::shared_ptr<Mesh> mesh,
                std::shared_ptr<Material> material)
        : zoneHalfSize_(config.zoneHalfSize)
        , movementSpeed_(config.movementSpeed)
        , rng_(45)
        , dist_(0.0f, 1.0f)
    {
        // Zone center: in front of camera
        Eigen::Vector3f forward = (camera.target - camera.position).normalized();
        zoneCenter_ = camera.position + forward * config.distance;

        // Spawn insects randomly within spread
        std::uniform_real_distribution<float> spawnDist(-config.spread, config.spread);

        for (int i = 0; i < config.count; ++i) {
            Transform t;
            t.position = zoneCenter_ + Eigen::Vector3f(
                spawnDist(rng_), spawnDist(rng_), spawnDist(rng_)
            );
            t.scale = Eigen::Vector3f(0.1f, 0.1f, 0.1f);

            insects_.push_back({mesh, t, material});
        }
    }

    void update() {
        for (auto& insect : insects_) {
            Eigen::Vector3f newPos = insect.transform.position + Eigen::Vector3f(
                (dist_(rng_) - 0.5f) * movementSpeed_,
                (dist_(rng_) - 0.5f) * movementSpeed_ * 0.2f,  // Less vertical
                (dist_(rng_) - 0.5f) * movementSpeed_
            );

            // Clamp to zone
            newPos.x() = std::clamp(newPos.x(), zoneCenter_.x() - zoneHalfSize_, zoneCenter_.x() + zoneHalfSize_);
            newPos.y() = std::clamp(newPos.y(), zoneCenter_.y() - zoneHalfSize_, zoneCenter_.y() + zoneHalfSize_);
            newPos.z() = std::clamp(newPos.z(), zoneCenter_.z() - zoneHalfSize_, zoneCenter_.z() + zoneHalfSize_);

            insect.transform.position = newPos;
        }
    }

    const std::vector<SceneObject>& getObjects() const { return insects_; }

private:
    std::vector<SceneObject> insects_;
    Eigen::Vector3f zoneCenter_;
    float zoneHalfSize_;
    float movementSpeed_;

    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_;
};

} // namespace scene
