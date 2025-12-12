#include <webgpu/webgpu_cpp.h>
#include "webgpu_utils.hpp"

class TriangleRenderer {
public:
    wgpu::RenderPipeline pipeline;

    TriangleRenderer() {};
    TriangleRenderer(
        wgpu::Device device,
        wgpu::TextureFormat colorFormat
    ) {
        createPipeline(device, colorFormat);
    }

    void createPipeline(
        wgpu::Device device,
        wgpu::TextureFormat colorFormat
    ) {
        auto module = utils::loadShaderModule(device, "triangle.wgsl");

        wgpu::VertexState vertex {.module = module};

        wgpu::ColorTargetState colorTarget {
            .format = colorFormat
        };

        wgpu::FragmentState fragment {
            .module = module,
            .targetCount = 1,
            .targets = &colorTarget,
        };

        wgpu::RenderPipelineDescriptor pipelineDesc {
            .label = "skybox",
            .vertex = vertex,
            .fragment = &fragment,
        };
        pipeline = device.CreateRenderPipeline(&pipelineDesc);
    }

    void render(
        wgpu::RenderPassEncoder pass
    ) {
        pass.SetPipeline(pipeline);
        pass.Draw(3);
    }
};
