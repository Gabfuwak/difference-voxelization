#pragma once

#include <webgpu/webgpu_cpp.h>
#include "context.hpp"

namespace core {

class Downsampler {
public:
    Downsampler(Context* ctx, wgpu::TextureFormat format)
        : ctx_(ctx), format_(format)
    {
        createSampler();
        createPipeline();
    }

    void downsample(wgpu::TextureView src, wgpu::TextureView dst,
                    uint32_t dstWidth, uint32_t dstHeight)
    {
        // Create bind group for this source texture
        std::array<wgpu::BindGroupEntry, 2> entries{};
        entries[0].binding = 0;
        entries[0].textureView = src;
        entries[1].binding = 1;
        entries[1].sampler = sampler_;

        wgpu::BindGroupDescriptor bgDesc{};
        bgDesc.layout = bindGroupLayout_;
        bgDesc.entryCount = entries.size();
        bgDesc.entries = entries.data();
        wgpu::BindGroup bindGroup = ctx_->device.CreateBindGroup(&bgDesc);

        // Render pass targeting dst
        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = dst;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0, 0, 0, 1};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &colorAttachment;

        wgpu::CommandEncoder encoder = ctx_->device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        pass.SetPipeline(pipeline_);
        pass.SetBindGroup(0, bindGroup);
        pass.SetViewport(0, 0, dstWidth, dstHeight, 0, 1);
        pass.Draw(3);  // Fullscreen triangle, no vertex buffer
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        ctx_->queue.Submit(1, &commands);
    }

private:
    Context* ctx_;
    wgpu::TextureFormat format_;
    wgpu::RenderPipeline pipeline_;
    wgpu::BindGroupLayout bindGroupLayout_;
    wgpu::Sampler sampler_;

    void createSampler() {
        wgpu::SamplerDescriptor desc{};
        desc.magFilter = wgpu::FilterMode::Linear;
        desc.minFilter = wgpu::FilterMode::Linear;
        desc.addressModeU = wgpu::AddressMode::ClampToEdge;
        desc.addressModeV = wgpu::AddressMode::ClampToEdge;
        sampler_ = ctx_->device.CreateSampler(&desc);
    }

    void createPipeline() {
        // Load shader
        std::string shaderCode = readShader("src/shaders/downsample.wgsl");
        wgpu::ShaderSourceWGSL wgsl{};
        wgsl.code = shaderCode.c_str();
        wgpu::ShaderModuleDescriptor shaderDesc{};
        shaderDesc.nextInChain = &wgsl;
        wgpu::ShaderModule shaderModule = ctx_->device.CreateShaderModule(&shaderDesc);

        // Bind group layout
        std::array<wgpu::BindGroupLayoutEntry, 2> layoutEntries{};
        layoutEntries[0].binding = 0;
        layoutEntries[0].visibility = wgpu::ShaderStage::Fragment;
        layoutEntries[0].texture.sampleType = wgpu::TextureSampleType::Float;
        layoutEntries[0].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        layoutEntries[1].binding = 1;
        layoutEntries[1].visibility = wgpu::ShaderStage::Fragment;
        layoutEntries[1].sampler.type = wgpu::SamplerBindingType::Filtering;

        wgpu::BindGroupLayoutDescriptor bglDesc{};
        bglDesc.entryCount = layoutEntries.size();
        bglDesc.entries = layoutEntries.data();
        bindGroupLayout_ = ctx_->device.CreateBindGroupLayout(&bglDesc);

        // Pipeline layout
        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = 1;
        plDesc.bindGroupLayouts = &bindGroupLayout_;
        wgpu::PipelineLayout pipelineLayout = ctx_->device.CreatePipelineLayout(&plDesc);

        // Vertex state (no buffers)
        wgpu::VertexState vertexState{};
        vertexState.module = shaderModule;
        vertexState.entryPoint = "vertexMain";
        vertexState.bufferCount = 0;

        // Fragment state
        wgpu::ColorTargetState colorTarget{};
        colorTarget.format = format_;

        wgpu::FragmentState fragmentState{};
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = "fragmentMain";
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        // Pipeline
        wgpu::RenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.layout = pipelineLayout;
        pipelineDesc.vertex = vertexState;
        pipelineDesc.fragment = &fragmentState;
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;

        pipeline_ = ctx_->device.CreateRenderPipeline(&pipelineDesc);
    }

    std::string readShader(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            throw std::runtime_error("Cannot open shader: " + path);
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
};

} // namespace core
