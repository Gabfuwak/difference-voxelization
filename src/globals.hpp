#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>

using namespace wgpu;

struct alignas(16) GlobalsData {
    glm::mat4 viewProjection;
    glm::mat4 viewProjectionInv;
    glm::vec3 position;
    // float pad;
};

class Globals {
public:
    GlobalsData data;
    Buffer buffer;
    BindGroup bindGroup;
    BindGroupLayout layout;

    void initialize(Device& device) {
        createLayout(device);
        createBuffer(device);
        createBindGroup(device);
    }

    void updateBuffer(Queue& queue) {
        queue.WriteBuffer(buffer, 0, &data, sizeof(GlobalsData));
    }

    void createBuffer(Device& device) {
        BufferDescriptor desc {
            .label = "globals",
            .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
            .size = sizeof(GlobalsData),
        };
        buffer = device.CreateBuffer(&desc);
    }

    void createLayout(Device& device) {
        std::vector<BindGroupLayoutEntry> entries {{
            .binding = 0,
            .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
            .buffer = {.type = BufferBindingType::Uniform},
        }};
        BindGroupLayoutDescriptor desc {
            .label = "globals",
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        layout = device.CreateBindGroupLayout(&desc);
    }

    void createBindGroup(Device& device) {
        std::vector<BindGroupEntry> entries {{
            .binding = 0,
            .buffer = buffer,
        }};
        BindGroupDescriptor desc {
            .label = "globals",
            .layout = layout,
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        bindGroup = device.CreateBindGroup(&desc);
    }
};
