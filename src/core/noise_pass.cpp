#include "core/noise_pass.hpp"

std::string NoisePass::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void NoisePass::init(wgpu::Device dev, wgpu::Queue q, wgpu::TextureFormat colorFmt, wgpu::TextureFormat depthFormat) {
    std::string shaderCode = readFile(SHADERS_DIR "/noise_pass.wgsl");
    wgpu::ShaderSourceWGSL wgsl{};
    wgsl.code = shaderCode.c_str();
    wgpu::ShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain = &wgsl;
    wgpu::ShaderModule shaderModule = dev.CreateShaderModule(&shaderDesc);

    device = dev;
    queue = q;

    wgpu::BindGroupLayoutEntry e{};
    e.binding = 0;
    e.visibility = wgpu::ShaderStage::Fragment;
    e.buffer.type = wgpu::BufferBindingType::Uniform;
    e.buffer.minBindingSize = sizeof(ParamsCPU);

    wgpu::BindGroupLayoutDescriptor bglDesc{};
    bglDesc.entryCount = 1;
    bglDesc.entries = &e;
    bgl = device.CreateBindGroupLayout(&bglDesc);

    wgpu::PipelineLayoutDescriptor plDesc{};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &bgl;
    auto pl = device.CreatePipelineLayout(&plDesc);

    wgpu::BufferDescriptor bd{};
    bd.size = sizeof(ParamsCPU);
    bd.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    paramsBuf = device.CreateBuffer(&bd);

    wgpu::BindGroupEntry bge{};
    bge.binding = 0;
    bge.buffer = paramsBuf;
    bge.offset = 0;
    bge.size = sizeof(ParamsCPU);

    wgpu::BindGroupDescriptor bgDesc{};
    bgDesc.layout = bgl;
    bgDesc.entryCount = 1;
    bgDesc.entries = &bge;
    bg = device.CreateBindGroup(&bgDesc);

    wgpu::BlendState blend{};
    blend.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
    blend.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blend.color.operation = wgpu::BlendOperation::Add;
    blend.alpha.srcFactor = wgpu::BlendFactor::Zero;
    blend.alpha.dstFactor = wgpu::BlendFactor::One;
    blend.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState cts{};
    cts.format = colorFmt;
    cts.blend = &blend;

    wgpu::FragmentState fs{};
    fs.module = shaderModule;
    fs.entryPoint = "fs_main";
    fs.targetCount = 1;
    fs.targets = &cts;

    wgpu::RenderPipelineDescriptor rp{};
    rp.layout = pl;
    rp.vertex.module = shaderModule;
    rp.vertex.entryPoint = "vs_main";
    rp.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    rp.fragment = &fs;

    pipeline = device.CreateRenderPipeline(&rp);
}

void NoisePass::render(wgpu::CommandEncoder enc, wgpu::TextureView outView, uint32_t w, uint32_t h, float time, float seed) {
    ParamsCPU p{};
    p.resolution[0] = float(w);
    p.resolution[1] = float(h);
    p.time = time;
    p.seed = seed;
    queue.WriteBuffer(paramsBuf, 0, &p, sizeof(p));

    wgpu::RenderPassColorAttachment ca{};
    ca.view = outView;
    ca.loadOp = wgpu::LoadOp::Load;
    ca.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor rpd{};
    rpd.colorAttachmentCount = 1;
    rpd.colorAttachments = &ca;

    auto pass = enc.BeginRenderPass(&rpd);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bg);
    pass.Draw(3);
    pass.End();
}
