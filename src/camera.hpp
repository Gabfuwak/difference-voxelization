#pragma once

#include <Eigen/Dense>
#include <cmath>

namespace scene {

class Camera {
public:
    Eigen::Vector3f position = {0.0f, 0.0f, 3.0f};
    Eigen::Vector3f target = {0.0f, 0.0f, 0.0f};
    Eigen::Vector3f up = {0.0f, 1.0f, 0.0f};

    float fov = 45.0f;        // degrees
    float aspect = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    Camera() = default;

    Camera(float aspectRatio) : aspect(aspectRatio) {}

    Eigen::Matrix4f getViewMatrix() const {
        // LookAt matrix
        Eigen::Vector3f f = (target - position).normalized();  // forward
        Eigen::Vector3f r = f.cross(up).normalized();          // right
        Eigen::Vector3f u = r.cross(f);                        // up

        Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
        view(0, 0) = r.x();  view(0, 1) = r.y();  view(0, 2) = r.z();
        view(1, 0) = u.x();  view(1, 1) = u.y();  view(1, 2) = u.z();
        view(2, 0) = -f.x(); view(2, 1) = -f.y(); view(2, 2) = -f.z();

        view(0, 3) = -r.dot(position);
        view(1, 3) = -u.dot(position);
        view(2, 3) = f.dot(position);

        return view;
    }

    Eigen::Matrix4f getProjectionMatrix() const {
        // Perspective projection matrix (WebGPU clip space: z in [0, 1])
        float fovRad = fov * M_PI / 180.0f;
        float tanHalfFov = std::tan(fovRad / 2.0f);

        Eigen::Matrix4f proj = Eigen::Matrix4f::Zero();
        proj(0, 0) = 1.0f / (aspect * tanHalfFov);
        proj(1, 1) = 1.0f / tanHalfFov;
        proj(2, 2) = farPlane / (nearPlane - farPlane);
        proj(2, 3) = (nearPlane * farPlane) / (nearPlane - farPlane);
        proj(3, 2) = -1.0f;

        return proj;
    }

    Eigen::Matrix4f getViewProjectionMatrix() const {
        return getProjectionMatrix() * getViewMatrix();
    }

    // Orbit around target
    void orbit(float deltaYaw, float deltaPitch) {
        Eigen::Vector3f offset = position - target;
        float radius = offset.norm();

        // Current angles
        float yaw = std::atan2(offset.x(), offset.z());
        float pitch = std::asin(offset.y() / radius);

        // Update angles
        yaw += deltaYaw;
        pitch += deltaPitch;

        // Clamp pitch
        pitch = std::clamp(pitch, -1.5f, 1.5f);

        // Convert back to position
        position.x() = target.x() + radius * std::cos(pitch) * std::sin(yaw);
        position.y() = target.y() + radius * std::sin(pitch);
        position.z() = target.z() + radius * std::cos(pitch) * std::cos(yaw);
    }

    void zoom(float delta) {
        Eigen::Vector3f offset = position - target;
        float radius = offset.norm();
        radius = std::max(0.5f, radius - delta);
        position = target + offset.normalized() * radius;
    }
};

} // namespace scene
