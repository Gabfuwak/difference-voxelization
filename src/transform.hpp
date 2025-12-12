#pragma once

#include <Eigen/Dense>
#include <Eigen/Geometry>

namespace scene {

class Transform {
public:
    Eigen::Vector3f position = Eigen::Vector3f::Zero();
    Eigen::Quaternionf rotation = Eigen::Quaternionf::Identity();
    Eigen::Vector3f scale = Eigen::Vector3f::Ones();

    Transform() = default;

    Transform(const Eigen::Vector3f& pos) : position(pos) {}

    Transform(const Eigen::Vector3f& pos, const Eigen::Quaternionf& rot)
        : position(pos), rotation(rot) {}

    Transform(const Eigen::Vector3f& pos, const Eigen::Quaternionf& rot, const Eigen::Vector3f& scl)
        : position(pos), rotation(rot), scale(scl) {}

    // Get the model matrix (scale -> rotate -> translate)
    Eigen::Matrix4f getMatrix() const {
        Eigen::Matrix4f mat = Eigen::Matrix4f::Identity();
        
        // Apply scale
        mat(0, 0) = scale.x();
        mat(1, 1) = scale.y();
        mat(2, 2) = scale.z();
        
        // Apply rotation
        Eigen::Matrix3f rotMat = rotation.toRotationMatrix();
        mat.block<3, 3>(0, 0) = rotMat * mat.block<3, 3>(0, 0);
        
        // Apply translation
        mat.block<3, 1>(0, 3) = position;
        
        return mat;
    }

    // Rotate by euler angles (radians)
    void setEulerAngles(float pitch, float yaw, float roll) {
        rotation = Eigen::AngleAxisf(yaw, Eigen::Vector3f::UnitY())
                 * Eigen::AngleAxisf(pitch, Eigen::Vector3f::UnitX())
                 * Eigen::AngleAxisf(roll, Eigen::Vector3f::UnitZ());
    }

    // Rotate around an axis
    void rotate(float angle, const Eigen::Vector3f& axis) {
        rotation = Eigen::AngleAxisf(angle, axis.normalized()) * rotation;
    }

    void translate(const Eigen::Vector3f& delta) {
        position += delta;
    }
};

} // namespace scene
