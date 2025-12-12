#pragma once

#include <limits>

#include <glm/glm.hpp>

class FreeCamera {
public:
    float pitch = 0.0f;
    float yaw = 0.0f;

    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 worldUp {0.0f, 1.0f, 0.0f};

    float fovy = glm::radians(45.0f);
    float zNear = 0.01f;
    float zFar = 1000.0f;
    float aspect = std::numeric_limits<float>::quiet_NaN();

    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;

    glm::mat4 view() const;
    glm::mat4 projection() const;
    glm::mat4 viewProjection() const;

    void pan(float dx, float dy, float speed);
    void zoom(float delta, float speed);
    void move(const glm::vec3& delta);
    void moveLocal(float forwardDelta, float rightDelta, float upDelta, float speed);
};
