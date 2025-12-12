#include <array>
#include <ranges>
#include <string>
#include <webgpu/webgpu_cpp.h>
#include "webgpu_utils.hpp"
#include "image.hpp"

using namespace wgpu;

class SkyboxRenderer {
public:
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroupLayout materialLayout;

    SkyboxRenderer() {}

    void initialize(
        wgpu::Device device,
        wgpu::TextureFormat colorFormat,
        wgpu::TextureFormat depthFormat,
        wgpu::BindGroupLayout globalsLayout
    ) {
        createMaterialLayout(device);
        createPipeline(device, colorFormat, depthFormat, globalsLayout);
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

    void createPipeline(
        wgpu::Device device,
        wgpu::TextureFormat colorFormat,
        wgpu::TextureFormat depthFormat,
        wgpu::BindGroupLayout globalsLayout
    ) {
        std::vector<BindGroupLayout> bindGroupLayouts = {
            globalsLayout,
            materialLayout,
        };
        PipelineLayoutDescriptor layoutDesc {
            .label = "skybox",
            .bindGroupLayoutCount = bindGroupLayouts.size(),
            .bindGroupLayouts = bindGroupLayouts.data(),
        };
        auto layout = device.CreatePipelineLayout(&layoutDesc);

        auto module = utils::loadShaderModule(device, "skybox.wgsl");

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

        wgpu::RenderPipelineDescriptor desc {
            .label = "skybox",
            .layout = layout,
            .vertex = vertex,
            .depthStencil = &depthStencil,
            .fragment = &fragment,
        };
        pipeline = device.CreateRenderPipeline(&desc);
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
    TextureView textureCubeView;

    void initialize(Device device, const std::array<std::string, 6>& facePaths) {
        createLayout(device);
        createSampler(device);
        createTextureCube(device, facePaths);
        createTextureCubeView();
        createBindGroup(device);
    }

    void createLayout(Device& device) {
        std::vector<BindGroupLayoutEntry> entries {{
            .binding = 0,
            .visibility = ShaderStage::Fragment,
            .texture = {.sampleType = TextureSampleType::Float, .viewDimension = TextureViewDimension::Cube},
        }, {
            .binding = 1,
            .visibility = ShaderStage::Fragment,
            .sampler = {.type = SamplerBindingType::Filtering},
        }};
        BindGroupLayoutDescriptor desc {
            .label = "skybox material",
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        layout = device.CreateBindGroupLayout(&desc);
    }

    void createTextureCube(Device& device, const std::array<std::string, 6>& facePaths) {
        auto faceImages = facePaths
            | std::views::transform([](auto path) { return Image::load(path); })
            | std::ranges::to<std::vector>();

        auto width = faceImages[0].getWidth();
        auto height = faceImages[0].getHeight();

        TextureDescriptor textureCubeDesc {
            .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
            .dimension = TextureDimension::e2D,
            .size = {width, height, 6},
            .format = TextureFormat::RGBA8Unorm,
        };
        textureCube = device.CreateTexture(&textureCubeDesc);

        TexelCopyBufferLayout dataLayout {
            .bytesPerRow = 4 * width,
            .rowsPerImage = height,
        };

        Extent3D writeSize {width, height};

        for (uint32_t layer = 0; layer < 6; layer++) {
            TexelCopyTextureInfo destination {
                .texture = textureCube,
                .origin = {0, 0, layer},
            };
            const auto dataSize = 4 * width * height;
            device.GetQueue().WriteTexture(&destination, faceImages[layer].data.get(), dataSize, &dataLayout, &writeSize);
        }
    }

    void createSampler(Device& device) {
        sampler = device.CreateSampler();
    }

    void createTextureCubeView() {
        TextureViewDescriptor desc {
            .dimension = TextureViewDimension::Cube,
        };
        textureCubeView = textureCube.CreateView(&desc);
    }

    void createBindGroup(Device& device) {
        std::vector<BindGroupEntry> entries {{
            .binding = 0,
            .textureView = textureCubeView,
        }, {
            .binding = 1,
            .sampler = sampler,
        }};
        BindGroupDescriptor desc {
            .layout = layout,
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        bindGroup = device.CreateBindGroup(&desc);
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
