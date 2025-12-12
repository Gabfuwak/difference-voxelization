#pragma once

#include <memory>
#include "mesh.hpp"
#include "material.hpp"
#include "transform.hpp"

namespace scene {

struct SceneObject {
    std::shared_ptr<Mesh> mesh;
    Transform transform;
    std::shared_ptr<Material> material;
};

} // namespace scene
