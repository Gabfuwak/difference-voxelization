#include <ranges>
#include <webgpu/webgpu_cpp.h>
#include "webgpu_utils.hpp"
#include "image.hpp"

using namespace wgpu;

class SkyboxRenderer {
public:
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroupLayout materialLayout;

    SkyboxRenderer() {};
    SkyboxRenderer(
        wgpu::Device device,
        wgpu::TextureFormat colorFormat,
        wgpu::TextureFormat depthFormat,
        wgpu::BindGroupLayout globalsLayout
    ) {
        createMaterialLayout(device);
        createPipeline(device, colorFormat, depthFormat, globalsLayout);
    }

    void createPipeline(
        wgpu::Device device,
        wgpu::TextureFormat colorFormat,
        wgpu::TextureFormat depthFormat,
        wgpu::BindGroupLayout globalsLayout
    ) {
        auto module = utils::loadShaderModule(device, "skybox.wgsl");

        std::vector layouts {globalsLayout, materialLayout};

        wgpu::VertexState vertex {.module = module};

        wgpu::DepthStencilState depthStencil {
            .format = wgpu::TextureFormat::Depth24Plus,
            .depthWriteEnabled = false,
            .depthCompare = wgpu::CompareFunction::LessEqual,
        };

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
            .depthStencil = &depthStencil,
            .fragment = &fragment,
        };
        device.CreateRenderPipeline(&pipelineDesc);
    }

    void render(
        wgpu::RenderPassEncoder pass,
        wgpu::BindGroup globalsBindGroup,
        wgpu::BindGroup materialBindGroup
    ) {
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, globalsBindGroup);
        pass.SetBindGroup(1, materialBindGroup);
        pass.Draw(3);
    }

    void createMaterialLayout(wgpu::Device device) {
        std::vector<wgpu::BindGroupLayoutEntry> entries {{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture = {
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::Cube,
            },
        }, {
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .sampler = {.type = wgpu::SamplerBindingType::Filtering}
        }};

        wgpu::BindGroupLayoutDescriptor layoutDesc {
            .label = "skybox material",
            .entryCount = entries.size(),
            .entries = entries.data(),
        };

        materialLayout = device.CreateBindGroupLayout(&layoutDesc);
    }
};

class SkyboxMaterial {
public:
    BindGroupLayout layout;
    BindGroup bindGroup;
    Sampler sampler;
    Texture textureCube;

    void initialize(Device& device) {
        createLayout(device);
        createSampler(device);
    };

    void createLayout(Device& device) {
        std::vector<BindGroupLayoutEntry> entries {{
            .binding = 0,
            .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
            .texture = {.sampleType = TextureSampleType::Float, .viewDimension = TextureViewDimension::Cube},
        }, {
            .binding = 1,
            .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
            .sampler = {.type = SamplerBindingType::Filtering},
        }};
        BindGroupLayoutDescriptor desc {
            .label = "skybox material",
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        layout = device.CreateBindGroupLayout(&desc);
    }

    void createBindGroup(Device& device) {
        // std::vector<BindGroupEntry> entries {{
        //     .binding = 0,
        //     .textureView = textureCubeView,
        // }, {
        //     .binding = 1,
        //     .sampler = sampler,
        // }};
        // BindGroupDescriptor desc {
        //     .layout = pipeline.GetBindGroupLayout(1),
        //     .entryCount = entries.size(),
        //     .entries = entries.data(),
        // };
        // materialBindGroup = device.CreateBindGroup(&desc);
    }

    void createSampler(Device& device) {
        sampler = device.CreateSampler();
    }

    void createTextureCube(Device& device, uint32_t length) {
        TextureDescriptor desc {
            .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
            .dimension = TextureDimension::e2D,
            .size = {length, length, 6},
            .format = TextureFormat::RGBA8Unorm,
        };
        textureCube = device.CreateTexture(&desc);
    }
};

class SkyboxMaterialLegacy {
    std::vector<Texture> faceTextures;
    Texture textureCube;
    TextureView textureCubeView;

    static Texture createTextureFromImage(Device& device, Image& image) {
        Extent3D textureSize {image.getWidth(), image.getHeight()};
            TextureDescriptor textureDesc {
                .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
                .size = textureSize,
                .format = TextureFormat::RGBA8Unorm,
            };
        auto texture = device.CreateTexture(&textureDesc);

        TexelCopyTextureInfo destination {
            .texture = texture,
        };
        TexelCopyBufferLayout textureDataLayout {
            .bytesPerRow = 4 * image.getWidth(),
            .rowsPerImage = image.getHeight(),
        };
        device.GetQueue().WriteTexture(&destination, image.data.get(), 4 * image.getWidth() * image.getHeight(), &textureDataLayout, &textureSize);

        return texture;
    }

    void createTextureCube(Device& device) {
        std::vector<Image> faceImages;
        faceImages.emplace_back(Image::load("leadenhall_market/pos-x.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/neg-x.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/pos-y.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/neg-y.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/pos-z.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/neg-z.jpg"));

        faceTextures = faceImages
            | std::views::transform([this, &device](auto& image) { return createTextureFromImage(device, image); })
            | std::ranges::to<std::vector>();

        uint32_t width = faceImages[0].getWidth();
        uint32_t height = faceImages[1].getHeight();

        TextureDescriptor textureCubeDesc {
            .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
            .dimension = TextureDimension::e2D,
            .size = {width, height, 6},
            .format = TextureFormat::RGBA8Unorm,
        };
        textureCube = device.CreateTexture(&textureCubeDesc);

        TexelCopyBufferLayout textureDataLayout {
            .bytesPerRow = 4 * width,
            .rowsPerImage = height,
        };
        Extent3D textureSize {width, height};

        for (uint32_t layer = 0; layer < 6; layer++) {
            auto& image = faceImages[layer];
            TexelCopyTextureInfo destination {
                .texture = textureCube,
                .origin = {0, 0, layer},
            };
            const auto dataSize = 4 * width * height;
            device.GetQueue().WriteTexture(&destination, image.data.get(), dataSize, &textureDataLayout, &textureSize);
        }

        TextureViewDescriptor textureCubeViewDesc {
            .dimension = TextureViewDimension::Cube,
        };
        textureCubeView = textureCube.CreateView(&textureCubeViewDesc);
    }
};
