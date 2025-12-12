#include <imgui.h>
#include <imgui_impl_wgpu.h>
#include <imgui_impl_glfw.h>
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>

#include "imgui_utils.hpp"
#include "webgpu_utils.hpp"

using namespace wgpu;

namespace utils {

void imguiNewFrame() {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplWGPU_NewFrame();
    ImGui::NewFrame();
}

void imguiInitialize(GLFWwindow* window, Device& device, Surface& surface) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplWGPU_InitInfo info;
    info.Device = device.Get();
    info.NumFramesInFlight = 3;
    info.RenderTargetFormat = static_cast<WGPUTextureFormat>(utils::getSurfaceTexture(surface).GetFormat());
    info.DepthStencilFormat = static_cast<WGPUTextureFormat>(TextureFormat::Undefined);
    ImGui_ImplWGPU_Init(&info);
}

void imguiRender(Device& device, Surface& surface) {
    CommandEncoderDescriptor commandEncoderDesc {};
    auto commandEncoder = device.CreateCommandEncoder(&commandEncoderDesc);

    auto surfaceTexture = utils::getSurfaceTexture(surface);
    std::vector<RenderPassColorAttachment> colorAttachments = {{
        .view = surfaceTexture.CreateView(),
        .loadOp = LoadOp::Load,
        .storeOp = StoreOp::Store,
    }};

    RenderPassDescriptor passDesc {
        .colorAttachmentCount = colorAttachments.size(),
        .colorAttachments = colorAttachments.data(),
    };
    auto pass = commandEncoder.BeginRenderPass(&passDesc);
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
    pass.End();

    std::vector<CommandBuffer> commands {commandEncoder.Finish()};
    device.GetQueue().Submit(commands.size(), commands.data());
}

void imguiRender(RenderPassEncoder& pass) {
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
}

} // namespace utils
