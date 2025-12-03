#pragma once

#include <fstream>
#include <sstream>
#include <vector>
#include <dawn/webgpu_cpp.h>
#include <opencv2/opencv.hpp>

#include "context.hpp"

namespace core {

class Renderer {
public:
    Context* ctx;
    uint32_t width;
    uint32_t height;

    wgpu::Texture targetTexture;
    wgpu::TextureView targetTextureView;
    wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm;

    wgpu::RenderPipeline pipeline;
    wgpu::BindGroupLayout bindGroupLayout;
    wgpu::BindGroup bindGroup;

    // Uniform buffer for MVP matrix
    wgpu::Buffer uniformBuffer;

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
        desc.size = {width, height, 1};
        desc.format = wgpu::TextureFormat::Depth24Plus;
        return ctx->device.CreateTexture(&desc);
    }

    void render(wgpu::Buffer vertexBuffer, wgpu::Buffer indexBuffer, 
                uint32_t indexCount, wgpu::TextureView depthView) {
        
        wgpu::TextureView colorView = targetTextureView;

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

    cv::Mat captureFrame() {
        // Calculate buffer size with padding
        uint32_t bytesPerRow = width * 4;  // RGBA = 4 bytes per pixel
        uint32_t paddedBytesPerRow = (bytesPerRow + 255) & ~255;  // Align to 256
        uint32_t bufferSize = paddedBytesPerRow * height;
        
        // Create staging buffer for reading
        wgpu::BufferDescriptor bufferDesc{};
        bufferDesc.label = "Staging buffer";
        bufferDesc.size = bufferSize;
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        wgpu::Buffer stagingBuffer = ctx->device.CreateBuffer(&bufferDesc);
        
        // Create command encoder to copy texture to buffer
        wgpu::CommandEncoder encoder = ctx->device.CreateCommandEncoder();
        
        // Source: texture
        wgpu::TexelCopyTextureInfo source{};
        source.texture = targetTexture;
        source.mipLevel = 0;
        source.origin = {0, 0, 0};
        source.aspect = wgpu::TextureAspect::All;
        
        // Destination: buffer with layout
        wgpu::TexelCopyBufferLayout layout{};
        layout.offset = 0;
        layout.bytesPerRow = paddedBytesPerRow;
        layout.rowsPerImage = height;
        
        wgpu::TexelCopyBufferInfo destination{};
        destination.buffer = stagingBuffer;
        destination.layout = layout;
        
        wgpu::Extent3D copySize = {width, height, 1};
        encoder.CopyTextureToBuffer(&source, &destination, &copySize);
        
        // Submit copy command
        wgpu::CommandBuffer commands = encoder.Finish();
        ctx->queue.Submit(1, &commands);
        
        // Wait for GPU work to complete before mapping
        bool workDone = false;
        ctx->queue.OnSubmittedWorkDone(
            wgpu::CallbackMode::AllowProcessEvents,
            [&workDone](wgpu::QueueWorkDoneStatus status, wgpu::StringView /*message*/) {
                workDone = true;
            }
        );
        
        while (!workDone) {
            ctx->instance.ProcessEvents();
        }
        
        // Map buffer
        bool mapped = false;
        stagingBuffer.MapAsync(
            wgpu::MapMode::Read,
            0,
            bufferSize,
            wgpu::CallbackMode::AllowProcessEvents,
            [&mapped](wgpu::MapAsyncStatus status, wgpu::StringView /*message*/) {
                mapped = (status == wgpu::MapAsyncStatus::Success);
            }
        );
        
        while (!mapped) {
            ctx->instance.ProcessEvents();
        }
        
        // Get mapped data
        const uint8_t* data = static_cast<const uint8_t*>(
            stagingBuffer.GetConstMappedRange(0, bufferSize));
        
        // Create OpenCV Mat (RGBA format)
        cv::Mat image(height, width, CV_8UC4);
        
        // Copy data row by row (handling padding)
        for (uint32_t y = 0; y < height; ++y) {
            memcpy(image.ptr(y), data + y * paddedBytesPerRow, bytesPerRow);
        }
        
        // Unmap buffer
        stagingBuffer.Unmap();
        
        // Convert RGBA to BGR for OpenCV
        cv::Mat bgr;
        cv::cvtColor(image, bgr, cv::COLOR_RGBA2BGR);
        
        return bgr;
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
