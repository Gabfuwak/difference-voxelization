#pragma once

#include <fstream>
#include <sstream>
#include <vector>
#include <dawn/webgpu_cpp.h>

#include "context.hpp"
#include "window.hpp"

namespace core {

class Renderer {
public:
    Context* ctx;
    Window* window;

    wgpu::RenderPipeline pipeline;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::BindGroup bindGroup;

    // Uniform buffer for MVP matrix
    wgpu::Buffer uniformBuffer;

    Renderer(Context* ctx, Window* window) : ctx(ctx), window(window) {}

    void createUniformBuffer(size_t size) {
        wgpu::BufferDescriptor desc{};
        desc.label = "Uniform Buffer";
        desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        desc.size = size;
        uniformBuffer = ctx->device.CreateBuffer(&desc);
    }

    void updateUniformBuffer(const void* data, size_t size) {
        ctx->queue.WriteBuffer(uniformBuffer, 0, data, size);
    }

    void createPipeline(const std::string& shaderPath) {
        // Load shader
        std::string shaderCode = readFile(shaderPath);
        wgpu::ShaderSourceWGSL wgsl{};
        wgsl.code = shaderCode.c_str();
        wgpu::ShaderModuleDescriptor shaderDesc{};
        shaderDesc.nextInChain = &wgsl;
        wgpu::ShaderModule shaderModule = ctx->device.CreateShaderModule(&shaderDesc);

        // Bind group layout - MVP matrix
        wgpu::BindGroupLayoutEntry layoutEntry{};
        layoutEntry.binding = 0;
        layoutEntry.visibility = wgpu::ShaderStage::Vertex;
        layoutEntry.buffer.type = wgpu::BufferBindingType::Uniform;

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
        bindGroupLayoutDesc.label = "Uniform bind group layout";
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries = &layoutEntry;
        bindGroupLayout = ctx->device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        // Bind group
        wgpu::BindGroupEntry bindGroupEntry{};
        bindGroupEntry.binding = 0;
        bindGroupEntry.buffer = uniformBuffer;
        bindGroupEntry.size = uniformBuffer.GetSize();

        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.label = "Uniform bind group";
        bindGroupDesc.layout = bindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &bindGroupEntry;
        bindGroup = ctx->device.CreateBindGroup(&bindGroupDesc);

        // Pipeline layout
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{};
        pipelineLayoutDesc.label = "Pipeline layout";
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        auto pipelineLayout = ctx->device.CreatePipelineLayout(&pipelineLayoutDesc);

        // Vertex attributes: position (vec3f) + color (vec3f)
        std::array<wgpu::VertexAttribute, 2> attributes{};
        attributes[0].format = wgpu::VertexFormat::Float32x3;
        attributes[0].offset = 0;
        attributes[0].shaderLocation = 0; // position

        attributes[1].format = wgpu::VertexFormat::Float32x3;
        attributes[1].offset = 3 * sizeof(float);
        attributes[1].shaderLocation = 1; // color

        wgpu::VertexBufferLayout vertexBufferLayout{};
        vertexBufferLayout.arrayStride = 6 * sizeof(float); // pos + color
        vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;
        vertexBufferLayout.attributeCount = attributes.size();
        vertexBufferLayout.attributes = attributes.data();

        // Vertex state
        wgpu::VertexState vertexState{};
        vertexState.module = shaderModule;
        vertexState.entryPoint = "vertexMain";
        vertexState.bufferCount = 1;
        vertexState.buffers = &vertexBufferLayout;

        // Fragment state
        wgpu::ColorTargetState colorTarget{};
        colorTarget.format = window->format;

        wgpu::FragmentState fragmentState{};
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = "fragmentMain";
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        // Depth stencil state
        wgpu::DepthStencilState depthStencil{};
        depthStencil.format = wgpu::TextureFormat::Depth24Plus;
        depthStencil.depthWriteEnabled = true;
        depthStencil.depthCompare = wgpu::CompareFunction::Less;

        // Primitive state
        wgpu::PrimitiveState primitive{};
        primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        primitive.cullMode = wgpu::CullMode::Back;

        // Render pipeline
        wgpu::RenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.label = "Render pipeline";
        pipelineDesc.layout = pipelineLayout;
        pipelineDesc.vertex = vertexState;
        pipelineDesc.primitive = primitive;
        pipelineDesc.depthStencil = &depthStencil;
        pipelineDesc.fragment = &fragmentState;
        pipeline = ctx->device.CreateRenderPipeline(&pipelineDesc);
    }

    wgpu::Texture createDepthTexture() {
        wgpu::TextureDescriptor desc{};
        desc.label = "Depth texture";
        desc.usage = wgpu::TextureUsage::RenderAttachment;
        desc.size = {window->width, window->height, 1};
        desc.format = wgpu::TextureFormat::Depth24Plus;
        return ctx->device.CreateTexture(&desc);
    }

    void render(wgpu::Buffer vertexBuffer, wgpu::Buffer indexBuffer, 
                uint32_t indexCount, wgpu::TextureView depthView) {
        
        wgpu::TextureView colorView = window->getCurrentTextureView();

        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = colorView;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.1, 0.1, 0.1, 1.0};

        wgpu::RenderPassDepthStencilAttachment depthAttachment{};
        depthAttachment.view = depthView;
        depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
        depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
        depthAttachment.depthClearValue = 1.0f;

        wgpu::RenderPassDescriptor renderPassDesc{};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = &depthAttachment;

        wgpu::CommandEncoder encoder = ctx->device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
        
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16);
        pass.DrawIndexed(indexCount);
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        ctx->queue.Submit(1, &commands);
    }

private:
    std::string readFile(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            throw std::runtime_error("Cannot open file: " + path);
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
};

} // namespace core
