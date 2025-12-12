#include <iostream>
#include <limits>
#include <webgpu/webgpu_cpp.h>
#include "GLFW/glfw3.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/matrix.hpp"
#include "imgui.h"
#include "imgui_utils.hpp"
#include "webgpu_utils.hpp"
#include <utils.hpp>
#include "stb_image.h"
#include <vector>
#include <ranges>
#include "utils.hpp"
#include <camera2.hpp>
#include <image.hpp>
#include <glfw_utils.hpp>
#include "skybox.hpp"
#include "globals.hpp"

using namespace wgpu;
using namespace glm;

struct Cursor {
    double lastX = std::numeric_limits<double>::quiet_NaN();
    double lastY = std::numeric_limits<double>::quiet_NaN();
    double deltaX = std::numeric_limits<double>::quiet_NaN();
    double deltaY = std::numeric_limits<double>::quiet_NaN();
    int deltaSkipCounter = 0;
    bool dragging = false;
};

class SkyboxApp {
public:
    std::string title = "Skybox App";
    uint32_t width = 640;
    uint32_t height = 480;

    Cursor cursor;

    GLFWwindow* window;
    Instance instance;
    Adapter adapter;
    Device device;
    Queue queue;
    Surface surface;
    RenderPipeline pipeline;
    BindGroup globalsBindGroup;
    BindGroup materialBindGroup;
    Texture texture;
    Sampler sampler;

    Globals globals;

    std::vector<Texture> faceTextures;
    Texture textureCube;
    TextureView textureCubeView;

    float time;
    Buffer timeBuffer;

    FreeCamera camera;
    Buffer cameraBuffer;

    SkyboxApp() {}

    void initialize() {
        if (!glfwInit()) {
            exit(1);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        instance = utils::createInstance();
        adapter = utils::requestAdapterSync(instance);
        device = utils::requestDeviceSync(instance, adapter);
        queue = device.GetQueue();

        createWindow();
        createSurface();

        utils::imguiInitialize(window, device, surface);

        createPipeline();
        createBuffers();
        // createTexture();
        createTextureCube();
        createSampler();
        createCamera();
        createCameraBuffer();
        // createBindGroup();
        createGlobalsBindGroup();
        createMaterialBindGroup();

        globals.initialize(device);
    }

    void createCameraBuffer() {
        BufferDescriptor cameraBufferDesc {
            .label = "camera",
            .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
            .size = sizeof(GlobalsData),
        };
        cameraBuffer = device.CreateBuffer(&cameraBufferDesc);
    }

    void createCamera() {
        camera = FreeCamera();
        auto [width, height] = utils::getFramebufferSize(window);
        camera.aspect = 1.0f * width / height;
    }

    void updateGlobals() {
        auto viewProjection = camera.viewProjection();
        auto viewProjectionInv = glm::inverse(viewProjection);
        globals.data.viewProjection = viewProjection;
        globals.data.viewProjectionInv = viewProjectionInv,
        globals.data.position = camera.position;
        globals.updateBuffer(queue);
    }

    void updateCameraBuffer() {
        auto viewProjection = camera.viewProjection();
        auto viewProjectionInv = glm::inverse(viewProjection);
        GlobalsData data {
            .viewProjection = viewProjection,
            .viewProjectionInv = viewProjectionInv,
            .position = camera.position,
        };
        queue.WriteBuffer(cameraBuffer, 0, &data, sizeof(GlobalsData));
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        auto* app = static_cast<SkyboxApp*>(glfwGetWindowUserPointer(window));
        if (app == nullptr) {
            return;
        }
        // app->onMouseButton(button, action, mods);
    }

    void onCursorPos(double x, double y) {
        if (cursor.dragging) {
            cursor.deltaX = x - cursor.lastX;
            cursor.deltaY = y - cursor.lastY;
        } else {
            cursor.deltaX = cursor.deltaY = 0.0f;
        }
        cursor.lastX = x;
        cursor.lastY = y;

        if (cursor.dragging) {
            if (!cursor.deltaSkipCounter) {
                camera.pan(cursor.deltaX, cursor.deltaY, 0.01f);
            } else {
                cursor.deltaSkipCounter--;
            }
        }
    }

    static void cursorPosCallback(GLFWwindow* window, double x, double y) {
        auto* app = static_cast<SkyboxApp*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        app->onCursorPos(x, y);
    }

    void onScroll(double dx, double dy) {
        camera.zoom(dy, 0.1);
    }

    void onMouseButton(int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                cursor.dragging = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwGetCursorPos(window, &cursor.lastX, &cursor.lastY);
                cursor.deltaSkipCounter = 2;
            } else if (action == GLFW_RELEASE) {
                cursor.dragging = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }

    void createBuffers() {
        BufferDescriptor bufferDesc {
            .usage = BufferUsage::Uniform | BufferUsage::CopyDst,
            .size = sizeof(float),
        };
        timeBuffer = device.CreateBuffer(&bufferDesc);
    }

    void createTexture() {
        auto image = Image::load("f_texture.png");

        Extent3D textureSize {image.getWidth(), image.getHeight()};
        TextureDescriptor textureDesc {
            .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
            .size = textureSize,
            .format = TextureFormat::RGBA8Unorm,
        };
        texture = device.CreateTexture(&textureDesc);

        TexelCopyTextureInfo destination {
            .texture = texture,
        };
        TexelCopyBufferLayout textureDataLayout {
            .bytesPerRow = 4 * image.getWidth(),
            .rowsPerImage = image.getHeight(),
        };
        queue.WriteTexture(&destination, image.data.get(), 4 * image.getWidth() * image.getHeight(), &textureDataLayout, &textureSize);
    }

    Texture createTextureFromImage(Image& image) {
        Extent3D textureSize {image.getWidth(), image.getHeight()};
            TextureDescriptor textureDesc {
                .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
                .size = textureSize,
                .format = TextureFormat::RGBA8Unorm,
            };
        auto texture = this->device.CreateTexture(&textureDesc);

        TexelCopyTextureInfo destination {
            .texture = texture,
        };
        TexelCopyBufferLayout textureDataLayout {
            .bytesPerRow = 4 * image.getWidth(),
            .rowsPerImage = image.getHeight(),
        };
        queue.WriteTexture(&destination, image.data.get(), 4 * image.getWidth() * image.getHeight(), &textureDataLayout, &textureSize);

        return texture;
    }

    void installGlfwCallbacks() {

    }

    void createTextureCube() {
        std::vector<Image> faceImages;
        faceImages.emplace_back(Image::load("leadenhall_market/pos-x.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/neg-x.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/pos-y.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/neg-y.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/pos-z.jpg"));
        faceImages.emplace_back(Image::load("leadenhall_market/neg-z.jpg"));

        faceTextures = faceImages
            | std::views::transform([this](auto& image) { return createTextureFromImage(image); })
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
            queue.WriteTexture(&destination, image.data.get(), dataSize, &textureDataLayout, &textureSize);
        }

        TextureViewDescriptor textureCubeViewDesc {
            .dimension = TextureViewDimension::Cube,
        };
        textureCubeView = textureCube.CreateView(&textureCubeViewDesc);
    }

    void createSampler() {
        SamplerDescriptor samplerDesc {};
        sampler = device.CreateSampler(&samplerDesc);
    }

    void start() {
        double startTime = glfwGetTime();

        time = 0;
        render();
        surface.Present();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            utils::imguiNewFrame();
            bool showDemo = true;
            ImGui::ShowDemoWindow(&showDemo);

            time = glfwGetTime() - startTime;
            render();
            utils::imguiRender(device, surface);
            surface.Present();
        }
    }

    void createPipeline() {
        auto shaderModule = utils::loadShaderModule(device, "skybox.wgsl");

        VertexState vertexState {
            .module = shaderModule,
        };

        auto format = utils::getSurfaceTexture(surface).GetFormat();
        std::vector<ColorTargetState> colorTargets {{
            .format = format,
        }};
        FragmentState fragmentState {
            .module = shaderModule,
            .targetCount = colorTargets.size(),
            .targets = colorTargets.data(),
        };


        // PipelineLayoutDescriptor layoutDesc {
        //     .label = "skybox",
        //     .bindGroupLayouts =
        // }

        RenderPipelineDescriptor pipelineDesc {
            // .layout =
            .vertex = vertexState,
            .fragment = &fragmentState,
        };
        pipeline = device.CreateRenderPipeline(&pipelineDesc);
    }

    void createGlobalsBindGroup() {
        std::vector<BindGroupEntry> entries {{
            .binding = 0,
            .buffer = cameraBuffer,
        }};
        BindGroupDescriptor desc {
            .layout = pipeline.GetBindGroupLayout(0),
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        globalsBindGroup = device.CreateBindGroup(&desc);
    }

    void createMaterialBindGroup() {
        assert(faceTextures.size() == 6);
        std::vector<BindGroupEntry> entries {{
            .binding = 0,
            .textureView = textureCubeView,
        }, {
            .binding = 1,
            .sampler = sampler,
        }};
        BindGroupDescriptor desc {
            .layout = pipeline.GetBindGroupLayout(1),
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        materialBindGroup = device.CreateBindGroup(&desc);
    }

    void updateBuffers() {
        queue.WriteBuffer(timeBuffer, 0, &time, sizeof(float));
    }

    void render() {
        // std::cout << time << '\n';
        updateBuffers();
        updateGlobals();
        updateCameraBuffer();

        CommandEncoderDescriptor commandEncoderDesc {};
        auto commandEncoder = device.CreateCommandEncoder(&commandEncoderDesc);

        auto surfaceTexture = utils::getSurfaceTexture(surface);
        std::vector<RenderPassColorAttachment> colorAttachments {{
            .view = surfaceTexture.CreateView(),
            .loadOp = LoadOp::Clear,
            .storeOp = StoreOp::Store,
            .clearValue = {1, 1, 1, 0},
        }};

        RenderPassDescriptor passDesc {
            .colorAttachmentCount = colorAttachments.size(),
            .colorAttachments = colorAttachments.data(),
        };
        auto pass = commandEncoder.BeginRenderPass(&passDesc);

        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, globalsBindGroup);
        // pass.SetBindGroup(0, globals.bindGroup);
        pass.SetBindGroup(1, materialBindGroup);
        pass.Draw(3);
        pass.End();

        std::vector<CommandBuffer> commands {{commandEncoder.Finish()}};
        queue.Submit(commands.size(), commands.data());
    }

    void createWindow() {
        window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, SkyboxApp::mouseButtonCallback);
        glfwSetCursorPosCallback(window, SkyboxApp::cursorPosCallback);
        glfwSetScrollCallback(window, SkyboxApp::scrollCallback);
    }

    static void scrollCallback(GLFWwindow* window, double dx, double dy) {
        auto app = static_cast<SkyboxApp*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        app->onScroll(dx, dy);
    }

    void createSurface() {
        surface = utils::createSurfaceWithPreferredFormat(device, window);
    }
};

int main() {
    auto app = SkyboxApp();
    app.initialize();
    app.start();

    return 0;
}
