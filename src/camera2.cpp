#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "camera2.hpp"

using namespace glm;

vec3 FreeCamera::forward() const {
    vec3 dir {
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch),
    };
    return normalize(dir);
}

vec3 FreeCamera::right() const {
    return normalize(cross(forward(), worldUp));
}

vec3 FreeCamera::up() const {
    return normalize(cross(right(), forward()));
}

mat4 FreeCamera::view() const {
    return lookAt(position, position + forward(), up());
}

mat4 FreeCamera::projection() const {
    return perspective(fovy, aspect, zNear, zFar);
}

mat4 FreeCamera::viewProjection() const {
    return projection() * view();
}

void FreeCamera::pan(float dx, float dy, float speed) {
    yaw += dx * speed;
    pitch += dy * speed;
    const float limit = radians(89.0f);
    pitch = clamp(pitch, -limit, limit);
}

void FreeCamera::zoom(float delta, float speed) {
    fovy *= std::exp(-delta * speed);
    fovy = clamp(fovy, radians(15.0f), radians(90.0f));
}

void FreeCamera::move(const vec3& delta) {
    position += delta;
}

void FreeCamera::moveLocal(float forwardDelta, float rightDelta, float upDelta, float speed) {
    position += (forward() * forwardDelta + right() * rightDelta + worldUp * upDelta) * speed;
}
