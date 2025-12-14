#pragma once

#include <iostream>
#include <webgpu/webgpu_cpp.h>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <string>

struct ParamsCPU {
    float resolution[2];
    float time;
    float seed;
};

struct NoisePass {
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroupLayout bgl;
    wgpu::BindGroup bg;
    wgpu::Buffer paramsBuf;

    std::string readFile(const std::string& path);
    void init(wgpu::Device dev, wgpu::Queue q, wgpu::TextureFormat colorFmt, wgpu::TextureFormat depthFormat);
    void render(wgpu::CommandEncoder enc, wgpu::TextureView outView, uint32_t w, uint32_t h, float time, float seed);
};
