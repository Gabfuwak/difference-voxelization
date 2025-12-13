#pragma once

#include <fstream>
#include <sstream>
#include <vector>
#include <dawn/webgpu_cpp.h>
#include <opencv2/opencv.hpp>
#include "scene/scene_object.hpp"
#include "scene/camera.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

#include "core/context.hpp"

namespace core {

class Renderer {
public:
    Context* ctx;
    uint32_t width;
    uint32_t height;

    wgpu::Texture targetTexture;
    wgpu::TextureView targetTextureView;
    wgpu::TextureFormat format = wgpu::TextureFormat::BGRA8Unorm;

    wgpu::RenderPipeline pipeline;
    wgpu::BindGroupLayout bindGroupLayout;

    // Uniform buffer for MVP matrix
    wgpu::Buffer uniformBuffer;

    bool wireframeMode = false;

    Renderer(Context* ctx, uint32_t width, uint32_t height)
        : ctx(ctx), width(width), height(height) {
        createRenderTarget();
    }

    void createRenderTarget() {
        wgpu::TextureDescriptor desc{};
        desc.label = "Render target";
        desc.dimension = wgpu::TextureDimension::e2D;
        desc.size = {width, height, 1};
        desc.format = format;
        desc.mipLevelCount = 1;
        desc.sampleCount = 1;
        desc.usage = wgpu::TextureUsage::RenderAttachment |
                     wgpu::TextureUsage::CopySrc;  // Important for reading back!

        targetTexture = ctx->device.CreateTexture(&desc);
        targetTextureView = targetTexture.CreateView();
    }

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


        std::array<wgpu::BindGroupLayoutEntry, 4> layoutEntries{};

        // Binding 0: MVP uniform
        layoutEntries[0].binding = 0;
        layoutEntries[0].visibility = wgpu::ShaderStage::Vertex;
        layoutEntries[0].buffer.type = wgpu::BufferBindingType::Uniform;

        // Binding 1: Texture
        layoutEntries[1].binding = 1;
        layoutEntries[1].visibility = wgpu::ShaderStage::Fragment;
        layoutEntries[1].texture.sampleType = wgpu::TextureSampleType::Float;
        layoutEntries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        // Binding 2: Sampler
        layoutEntries[2].binding = 2;
        layoutEntries[2].visibility = wgpu::ShaderStage::Fragment;
        layoutEntries[2].sampler.type = wgpu::SamplerBindingType::Filtering;

        // Binding 3: Mask texture (NEW)
        layoutEntries[3].binding = 3;
        layoutEntries[3].visibility = wgpu::ShaderStage::Fragment;
        layoutEntries[3].texture.sampleType = wgpu::TextureSampleType::Float;
        layoutEntries[3].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
        bindGroupLayoutDesc.label = "Uniform + Texture bind group layout";
        bindGroupLayoutDesc.entryCount = layoutEntries.size();
        bindGroupLayoutDesc.entries = layoutEntries.data();
        bindGroupLayout = ctx->device.CreateBindGroupLayout(&bindGroupLayoutDesc);

        // Pipeline layout
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{};
        pipelineLayoutDesc.label = "Pipeline layout";
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        auto pipelineLayout = ctx->device.CreatePipelineLayout(&pipelineLayoutDesc);

        // Vertex attributes: position (vec3f) + color (vec3f)
        std::array<wgpu::VertexAttribute, 3> attributes{};  // Changed from 2 to 3
        attributes[0].format = wgpu::VertexFormat::Float32x3;
        attributes[0].offset = 0;
        attributes[0].shaderLocation = 0; // position

        attributes[1].format = wgpu::VertexFormat::Float32x3;
        attributes[1].offset = 3 * sizeof(float);
        attributes[1].shaderLocation = 1; // color

        attributes[2].format = wgpu::VertexFormat::Float32x2;  // NEW: UV is 2 floats
        attributes[2].offset = 6 * sizeof(float);              // NEW: after pos + color
        attributes[2].shaderLocation = 2;                      // NEW: location 2

        wgpu::VertexBufferLayout vertexBufferLayout{};
        vertexBufferLayout.arrayStride = 8 * sizeof(float); // pos + color + uv (was 6)
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
        colorTarget.format = format;

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
        if (wireframeMode) {
            primitive.topology = wgpu::PrimitiveTopology::LineList;
        } else {
            primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        }
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
        desc.size = {width, height, 1};
        desc.format = wgpu::TextureFormat::Depth24Plus;
        return ctx->device.CreateTexture(&desc);
    }

    void renderScene(const std::vector<scene::SceneObject>& objects,
                     const scene::Camera& camera,
                     wgpu::TextureView depthView, wgpu::TextureView targetView = nullptr, bool imgui = false) {

        for (size_t i = 0; i < objects.size(); ++i) {
            const auto& obj = objects[i];

            // Compute MVP for this object
            Eigen::Matrix4f mvp = camera.getViewProjectionMatrix() * obj.transform.getMatrix();
            updateUniformBuffer(mvp.data(), sizeof(float) * 16);

            // Draw (pass material's bind group)
            render(obj.mesh->vertexBuffer, obj.mesh->indexBuffer,
                   obj.mesh->indexCount, depthView, i == 0,
                   obj.material->bindGroup,  // ADD THIS
                   targetView, imgui);
        }
    }


    void clear(wgpu::TextureView targetView) {
        auto commandEncoder = ctx->device.CreateCommandEncoder();
        wgpu::RenderPassColorAttachment colorAttachment {
            .view =  targetView,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = {0.5, 0.5, 0.5, 1},
        };

        wgpu::RenderPassDescriptor passDesc {
            .colorAttachmentCount = 1,
            .colorAttachments = &colorAttachment,
        };
        auto pass = commandEncoder.BeginRenderPass(&passDesc);
        pass.End();

        auto command = commandEncoder.Finish();
        ctx->queue.Submit(1, &command);
    }

    void renderImgui(wgpu::TextureView depthView, wgpu::TextureView targetView, bool clear = false) {
        auto commandEncoder = ctx->device.CreateCommandEncoder();
        wgpu::RenderPassColorAttachment colorAttachment {
            .view = targetView,
            .loadOp = clear ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = {0.1, 0.1, 0.1, 1.0},
        };

        wgpu::RenderPassDepthStencilAttachment depthAttachment{};
        depthAttachment.view = depthView;
        depthAttachment.depthLoadOp = clear ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
        depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
        depthAttachment.depthClearValue = 1.0f;

        wgpu::RenderPassDescriptor passDesc {
            .colorAttachmentCount = 1,
            .colorAttachments = &colorAttachment,
            .depthStencilAttachment = &depthAttachment,
        };
        auto pass = commandEncoder.BeginRenderPass(&passDesc);
        
        ImGui::Render();
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
        
        pass.End();

        auto command = commandEncoder.Finish();
        ctx->queue.Submit(1, &command);
    }
    

private:

    void render(wgpu::Buffer vertexBuffer, wgpu::Buffer indexBuffer,
                uint32_t indexCount, wgpu::TextureView depthView, bool clear, wgpu::BindGroup materialBindGroup, wgpu::TextureView targetView = nullptr, bool imgui = false) {

        wgpu::TextureView colorView = targetView != nullptr ? targetView : targetTextureView;

        wgpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.view = colorView;
        colorAttachment.loadOp = clear ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = {0.1, 0.1, 0.1, 1.0};

        wgpu::RenderPassDepthStencilAttachment depthAttachment{};
        depthAttachment.view = depthView;
        depthAttachment.depthLoadOp = clear ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
        depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
        depthAttachment.depthClearValue = 1.0f;

        wgpu::RenderPassDescriptor renderPassDesc{};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        renderPassDesc.depthStencilAttachment = &depthAttachment;

        wgpu::CommandEncoder encoder = ctx->device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, materialBindGroup);
        pass.SetVertexBuffer(0, vertexBuffer);
        pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16);
        pass.DrawIndexed(indexCount);

        if (imgui) {
            ImGui::Render();
            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
        }

        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        ctx->queue.Submit(1, &commands);
    }

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
