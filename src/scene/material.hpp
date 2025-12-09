#pragma once
#include <string>
#include <webgpu/webgpu_cpp.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Material {
public:
    wgpu::Texture texture;
    wgpu::TextureView textureView;
    wgpu::Sampler sampler;
    wgpu::BindGroup bindGroup;
    bool hasTexture = false;

    static Material create(wgpu::Device device, wgpu::Queue queue, 
                          const std::string& texturePath) {
        Material mat;
        mat.hasTexture = true;
        
        // Load image
        int width, height, channels;
        unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);
        if (!data) {
            throw std::runtime_error("Failed to load texture: " + texturePath);
        }
        
        // Create texture
        wgpu::TextureDescriptor texDesc{};
        texDesc.size = {(uint32_t)width, (uint32_t)height, 1};
        texDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        texDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        texDesc.mipLevelCount = 1;
        mat.texture = device.CreateTexture(&texDesc);
        
        // Upload data
        wgpu::TexelCopyBufferLayout dataLayout{};
        dataLayout.bytesPerRow = width * 4;
        dataLayout.bytesPerRow = width * 4;
        dataLayout.rowsPerImage = height;
        
        wgpu::TexelCopyTextureInfo destination{};
        destination.texture = mat.texture;
        destination.mipLevel = 0;
        
        wgpu::Extent3D writeSize = {(uint32_t)width, (uint32_t)height, 1};
        queue.WriteTexture(&destination, data, width * height * 4, &dataLayout, &writeSize);
        
        stbi_image_free(data);
        
        // Create view
        mat.textureView = mat.texture.CreateView();
        
        // Create sampler
        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
        samplerDesc.magFilter = wgpu::FilterMode::Linear;
        samplerDesc.minFilter = wgpu::FilterMode::Linear;
        mat.sampler = device.CreateSampler(&samplerDesc);
        
        return mat;
    }
    
    static Material createUntextured(wgpu::Device device, wgpu::Queue queue) {
        Material mat;
        mat.hasTexture = false;
        
        // Create 1x1 white texture as fallback
        wgpu::TextureDescriptor texDesc{};
        texDesc.size = {1, 1, 1};
        texDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        texDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        texDesc.mipLevelCount = 1;
        mat.texture = device.CreateTexture(&texDesc);
        
        // White pixel
        uint32_t white = 0xFFFFFFFF;
        wgpu::TexelCopyBufferLayout dataLayout{};
        dataLayout.bytesPerRow = 4;
        
        wgpu::TexelCopyTextureInfo destination{};
        destination.texture = mat.texture;
        
        wgpu::Extent3D writeSize = {1, 1, 1};
        queue.WriteTexture(&destination, &white, 4, &dataLayout, &writeSize);
        
        mat.textureView = mat.texture.CreateView();
        
        wgpu::SamplerDescriptor samplerDesc{};
        mat.sampler = device.CreateSampler(&samplerDesc);
        
        return mat;
    }

    void createBindGroup(wgpu::Device device, wgpu::BindGroupLayout layout, 
                        wgpu::Buffer uniformBuffer) {
        std::array<wgpu::BindGroupEntry, 3> entries{};
        
        // Binding 0: Uniform buffer (MVP)
        entries[0].binding = 0;
        entries[0].buffer = uniformBuffer;
        entries[0].size = uniformBuffer.GetSize();
        
        // Binding 1: Texture view
        entries[1].binding = 1;
        entries[1].textureView = textureView;
        
        // Binding 2: Sampler
        entries[2].binding = 2;
        entries[2].sampler = sampler;
        
        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout = layout;
        bindGroupDesc.entryCount = entries.size();
        bindGroupDesc.entries = entries.data();
        
        bindGroup = device.CreateBindGroup(&bindGroupDesc);
    }
};
