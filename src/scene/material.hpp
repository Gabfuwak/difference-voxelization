#pragma once
#include <string>
#include <webgpu/webgpu_cpp.h>
#include <Eigen/Dense>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Material {
public:
    wgpu::Texture texture;
    wgpu::TextureView textureView;
    wgpu::Sampler sampler;
    wgpu::BindGroup bindGroup;
    bool hasTexture = false;
    
    // Mask support
    wgpu::Texture maskTexture;
    wgpu::TextureView maskTextureView;
    bool hasMask = false;

    static Material create(wgpu::Device device, wgpu::Queue queue, 
                          const std::string& texturePath,
                          const std::string& maskPath = "") {
        Material mat;
        mat.hasTexture = true;
        
        // Load diffuse texture (same as before)
        int width, height, channels;
        unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);
        if (!data) {
            throw std::runtime_error("Failed to load texture: " + texturePath);
        }
        
        wgpu::TextureDescriptor texDesc{};
        texDesc.size = {(uint32_t)width, (uint32_t)height, 1};
        texDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        texDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        texDesc.mipLevelCount = 1;
        mat.texture = device.CreateTexture(&texDesc);
        
        wgpu::TexelCopyBufferLayout dataLayout{};
        dataLayout.bytesPerRow = width * 4;
        dataLayout.rowsPerImage = height;
        
        wgpu::TexelCopyTextureInfo destination{};
        destination.texture = mat.texture;
        wgpu::Extent3D writeSize{(uint32_t)width, (uint32_t)height, 1};
        queue.WriteTexture(&destination, data, width * height * 4, &dataLayout, &writeSize);
        
        stbi_image_free(data);
        mat.textureView = mat.texture.CreateView();
        
        // Create sampler
        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
        samplerDesc.magFilter = wgpu::FilterMode::Linear;
        samplerDesc.minFilter = wgpu::FilterMode::Linear;
        mat.sampler = device.CreateSampler(&samplerDesc);
        
        // Load mask if provided
        if (!maskPath.empty()) {
            mat.hasMask = true;
            int maskWidth, maskHeight, maskChannels;
            unsigned char* maskData = stbi_load(maskPath.c_str(), &maskWidth, &maskHeight, 
                                               &maskChannels, 1); // Force 1 channel
            
            wgpu::TextureDescriptor maskTexDesc{};
            maskTexDesc.size = {(uint32_t)maskWidth, (uint32_t)maskHeight, 1};
            maskTexDesc.format = wgpu::TextureFormat::R8Unorm;
            maskTexDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
            maskTexDesc.mipLevelCount = 1;
            mat.maskTexture = device.CreateTexture(&maskTexDesc);
            
            wgpu::TexelCopyBufferLayout maskLayout{};
            maskLayout.bytesPerRow = maskWidth;
            
            wgpu::TexelCopyTextureInfo maskDest{};
            maskDest.texture = mat.maskTexture;
            wgpu::Extent3D maskWriteSize{(uint32_t)maskWidth, (uint32_t)maskHeight, 1};
            queue.WriteTexture(&maskDest, maskData, maskWidth * maskHeight, &maskLayout, &maskWriteSize);
            
            stbi_image_free(maskData);
            mat.maskTextureView = mat.maskTexture.CreateView();
        }
        
        return mat;
    }
    
    static Material createUntextured(wgpu::Device device, wgpu::Queue queue) {
        Material mat;
        mat.hasTexture = false;
        
        // Create 1x1 white texture
        wgpu::TextureDescriptor texDesc{};
        texDesc.size = {1, 1, 1};
        texDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        texDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        texDesc.mipLevelCount = 1;
        mat.texture = device.CreateTexture(&texDesc);
        
        uint32_t white = 0xFFFFFFFF;
        wgpu::TexelCopyBufferLayout dataLayout{};
        dataLayout.bytesPerRow = 4;
        wgpu::TexelCopyTextureInfo destination{};
        destination.texture = mat.texture;
        wgpu::Extent3D writeSize{1, 1, 1};
        queue.WriteTexture(&destination, &white, 4, &dataLayout, &writeSize);
        
        mat.textureView = mat.texture.CreateView();
        mat.sampler = device.CreateSampler({});
        
        return mat;
    }

    static Material createColored(wgpu::Device device, wgpu::Queue queue, 
                             const Eigen::Vector3f& color) {
        Material mat;
        mat.hasTexture = false;
        
        // Create 1x1 texture with the specified color
        wgpu::TextureDescriptor texDesc{};
        texDesc.size = {1, 1, 1};
        texDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        texDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        texDesc.mipLevelCount = 1;
        mat.texture = device.CreateTexture(&texDesc);
        
        // Convert float color (0-1) to uint8 (0-255)
        uint8_t r = static_cast<uint8_t>(color.x() * 255);
        uint8_t g = static_cast<uint8_t>(color.y() * 255);
        uint8_t b = static_cast<uint8_t>(color.z() * 255);
        uint32_t colorValue = (255 << 24) | (b << 16) | (g << 8) | r; // RGBA
        
        wgpu::TexelCopyBufferLayout dataLayout{};
        dataLayout.bytesPerRow = 4;
        wgpu::TexelCopyTextureInfo destination{};
        destination.texture = mat.texture;
        wgpu::Extent3D writeSize{1, 1, 1};
        queue.WriteTexture(&destination, &colorValue, 4, &dataLayout, &writeSize);
        
        mat.textureView = mat.texture.CreateView();
        mat.sampler = device.CreateSampler({});
        
        return mat;
    }

    void createBindGroup(wgpu::Device device, wgpu::BindGroupLayout layout, 
                        wgpu::Buffer uniformBuffer, wgpu::TextureView dummyMaskView) {
        std::array<wgpu::BindGroupEntry, 4> entries{};
        
        entries[0].binding = 0;
        entries[0].buffer = uniformBuffer;
        entries[0].size = uniformBuffer.GetSize();
        
        entries[1].binding = 1;
        entries[1].textureView = textureView;
        
        entries[2].binding = 2;
        entries[2].sampler = sampler;
        
        // Use real mask or dummy
        entries[3].binding = 3;
        entries[3].textureView = hasMask ? maskTextureView : dummyMaskView;
        
        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout = layout;
        bindGroupDesc.entryCount = entries.size();
        bindGroupDesc.entries = entries.data();
        
        bindGroup = device.CreateBindGroup(&bindGroupDesc);
    }
};
