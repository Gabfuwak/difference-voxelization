#include <webgpu/webgpu_cpp.h>
#include <cstdint>
#include <cstring>

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

    void init(wgpu::Device dev, wgpu::Queue q, wgpu::TextureFormat colorFmt, wgpu::ShaderModule shader) {
        device = dev; queue = q;

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

        wgpu::ColorTargetState cts{};
        cts.format = colorFmt;

        wgpu::FragmentState fs{};
        fs.module = shader;
        fs.entryPoint = "fs_main";
        fs.targetCount = 1;
        fs.targets = &cts;

        wgpu::RenderPipelineDescriptor rp{};
        rp.layout = pl;
        rp.vertex.module = shader;
        rp.vertex.entryPoint = "vs_main";
        rp.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        rp.fragment = &fs;

        pipeline = device.CreateRenderPipeline(&rp);
    }

    void render(wgpu::CommandEncoder enc, wgpu::TextureView outView, uint32_t w, uint32_t h, float time, float seed) {
        ParamsCPU p{};
        p.resolution[0] = float(w);
        p.resolution[1] = float(h);
        p.time = time;
        p.seed = seed;
        queue.WriteBuffer(paramsBuf, 0, &p, sizeof(p));

        wgpu::RenderPassColorAttachment ca{};
        ca.view = outView;
        ca.loadOp = wgpu::LoadOp::Clear;
        ca.storeOp = wgpu::StoreOp::Store;
        ca.clearValue = {0, 0, 0, 1};

        wgpu::RenderPassDescriptor rpd{};
        rpd.colorAttachmentCount = 1;
        rpd.colorAttachments = &ca;

        auto pass = enc.BeginRenderPass(&rpd);
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bg);
        pass.Draw(3);
        pass.End();
    }
};
