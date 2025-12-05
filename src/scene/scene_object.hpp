#pragma once

#include <memory>
#include "mesh.hpp"
#include "transform.hpp"

namespace scene {

struct SceneObject {
    std::shared_ptr<Mesh> mesh;
    Transform transform;
};

} // namespace scene
