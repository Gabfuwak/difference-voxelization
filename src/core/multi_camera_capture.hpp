#pragma once

#include <vector>
#include <algorithm>
#include <cassert>
#include <webgpu/webgpu_cpp.h>
#include <opencv2/opencv.hpp>
#include "core/context.hpp"
#include "core/renderer.hpp"
#include "core/downsampler.hpp"
#include "scene/scene_object.hpp"
#include "scene/camera.hpp"

namespace core {

struct CaptureTarget {
    // High-res (supersample factor applied)
    wgpu::Texture renderTexture;
    wgpu::TextureView renderView;
    wgpu::Texture depthTexture;
    wgpu::TextureView depthView;

    // Output resolution (what caller asked for)
    wgpu::Texture outputTexture;
    wgpu::TextureView outputView;

    wgpu::Buffer stagingBuffer;

    uint32_t width;          // output width
    uint32_t height;         // output height
    uint32_t renderWidth;    // high-res width
    uint32_t renderHeight;   // high-res height
    uint32_t paddedBytesPerRow;
    uint32_t bufferSize;
};

class MultiCameraCapture {
public:
    MultiCameraCapture(Context* ctx, uint32_t cameraCount,
                       uint32_t width, uint32_t height, uint32_t supersample = 2)
        : ctx_(ctx), width_(width), height_(height), supersample_(supersample),
          downsampler_(ctx, wgpu::TextureFormat::BGRA8Unorm)
    {
        targets_.resize(cameraCount);
        for (auto& target : targets_) {
            initializeTarget(target);
        }
    }

    void renderAll(
        const std::vector<scene::Camera>& cameras,
        const std::vector<scene::SceneObject>& objects,
        Renderer& renderer
    ) {
        assert(cameras.size() == targets_.size());

        for (size_t i = 0; i < cameras.size(); ++i) {
            renderer.renderScene(objects, cameras[i],
                                targets_[i].depthView,
                                targets_[i].renderView);
        }
    }

    void downsampleAll() {
        for (auto& target : targets_) {
            downsampler_.downsample(target.renderView, target.outputView,
                                    target.width, target.height);
        }
    }

    void noiseAll() {}

    void copyAll() {
        wgpu::CommandEncoder encoder = ctx_->device.CreateCommandEncoder();

        for (auto& target : targets_) {
            wgpu::TexelCopyTextureInfo source{};
            source.texture = target.outputTexture;  // Changed from renderTexture
            source.mipLevel = 0;
            source.origin = {0, 0, 0};
            source.aspect = wgpu::TextureAspect::All;

            wgpu::TexelCopyBufferLayout layout{};
            layout.offset = 0;
            layout.bytesPerRow = target.paddedBytesPerRow;
            layout.rowsPerImage = target.height;

            wgpu::TexelCopyBufferInfo destination{};
            destination.buffer = target.stagingBuffer;
            destination.layout = layout;

            wgpu::Extent3D copySize = {target.width, target.height, 1};
            encoder.CopyTextureToBuffer(&source, &destination, &copySize);
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        ctx_->queue.Submit(1, &commands);
    }

    void sync() {
        bool done = false;
        ctx_->queue.OnSubmittedWorkDone(
            wgpu::CallbackMode::AllowProcessEvents,
            [&done](wgpu::QueueWorkDoneStatus, wgpu::StringView) {
                done = true;
            }
        );
        while (!done) {
            ctx_->instance.ProcessEvents();
        }
    }

    std::vector<cv::Mat> readAll() {
        std::vector<cv::Mat> results;
        results.reserve(targets_.size());

        std::vector<bool> mapped(targets_.size(), false);

        for (size_t i = 0; i < targets_.size(); ++i) {
            targets_[i].stagingBuffer.MapAsync(
                wgpu::MapMode::Read,
                0,
                targets_[i].bufferSize,
                wgpu::CallbackMode::AllowProcessEvents,
                [&mapped, i](wgpu::MapAsyncStatus status, wgpu::StringView) {
                    mapped[i] = (status == wgpu::MapAsyncStatus::Success);
                }
            );
        }

        auto allMapped = [&mapped]() {
            return std::all_of(mapped.begin(), mapped.end(), [](bool b) { return b; });
        };

        while (!allMapped()) {
            ctx_->instance.ProcessEvents();
        }

        for (size_t i = 0; i < targets_.size(); ++i) {
            results.push_back(readTarget(targets_[i]));
        }

        return results;
    }

    size_t cameraCount() const { return targets_.size(); }
    const CaptureTarget& getTarget(size_t index) const { return targets_[index]; }
    uint32_t renderWidth() const { return width_ * supersample_; }
    uint32_t renderHeight() const { return height_ * supersample_; }

private:
    Context* ctx_;
    std::vector<CaptureTarget> targets_;
    Downsampler downsampler_;
    uint32_t width_;
    uint32_t height_;
    uint32_t supersample_;

    void initializeTarget(CaptureTarget& target) {
        target.width = width_;
        target.height = height_;
        target.renderWidth = width_ * supersample_;
        target.renderHeight = height_ * supersample_;

        uint32_t bytesPerRow = width_ * 4;
        target.paddedBytesPerRow = (bytesPerRow + 255) & ~255;
        target.bufferSize = target.paddedBytesPerRow * height_;

        // High-res render texture
        wgpu::TextureDescriptor renderDesc{};
        renderDesc.label = "Capture render texture (high-res)";
        renderDesc.dimension = wgpu::TextureDimension::e2D;
        renderDesc.size = {target.renderWidth, target.renderHeight, 1};
        renderDesc.format = wgpu::TextureFormat::BGRA8Unorm;
        renderDesc.mipLevelCount = 1;
        renderDesc.sampleCount = 1;
        renderDesc.usage = wgpu::TextureUsage::RenderAttachment |
                          wgpu::TextureUsage::TextureBinding;  // Changed for sampling
        target.renderTexture = ctx_->device.CreateTexture(&renderDesc);
        target.renderView = target.renderTexture.CreateView();

        // High-res depth texture
        wgpu::TextureDescriptor depthDesc{};
        depthDesc.label = "Capture depth texture (high-res)";
        depthDesc.dimension = wgpu::TextureDimension::e2D;
        depthDesc.size = {target.renderWidth, target.renderHeight, 1};
        depthDesc.format = wgpu::TextureFormat::Depth24Plus;
        depthDesc.mipLevelCount = 1;
        depthDesc.sampleCount = 1;
        depthDesc.usage = wgpu::TextureUsage::RenderAttachment;
        target.depthTexture = ctx_->device.CreateTexture(&depthDesc);
        target.depthView = target.depthTexture.CreateView();

        // Output texture (final resolution)
        wgpu::TextureDescriptor outputDesc{};
        outputDesc.label = "Capture output texture";
        outputDesc.dimension = wgpu::TextureDimension::e2D;
        outputDesc.size = {target.width, target.height, 1};
        outputDesc.format = wgpu::TextureFormat::BGRA8Unorm;
        outputDesc.mipLevelCount = 1;
        outputDesc.sampleCount = 1;
        outputDesc.usage = wgpu::TextureUsage::RenderAttachment |
                          wgpu::TextureUsage::CopySrc;
        target.outputTexture = ctx_->device.CreateTexture(&outputDesc);
        target.outputView = target.outputTexture.CreateView();

        // Staging buffer (output resolution)
        wgpu::BufferDescriptor bufferDesc{};
        bufferDesc.label = "Capture staging buffer";
        bufferDesc.size = target.bufferSize;
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        target.stagingBuffer = ctx_->device.CreateBuffer(&bufferDesc);
    }

    cv::Mat readTarget(CaptureTarget& target) {
        const uint8_t* data = static_cast<const uint8_t*>(
            target.stagingBuffer.GetConstMappedRange(0, target.bufferSize));

        cv::Mat image(target.height, target.width, CV_8UC4);
        uint32_t bytesPerRow = target.width * 4;

        for (uint32_t y = 0; y < target.height; ++y) {
            memcpy(image.ptr(y), data + y * target.paddedBytesPerRow, bytesPerRow);
        }

        target.stagingBuffer.Unmap();

        cv::Mat bgr;
        cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
        return bgr;
    }
};

} // namespace core
