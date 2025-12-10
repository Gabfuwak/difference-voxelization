#include <iostream>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

wgpu::Texture getSurfaceTexture(const wgpu::Surface& surface) {
    wgpu::SurfaceTexture surfaceTexture {};
    surface.GetCurrentTexture(&surfaceTexture);

    if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal) {
        std::cerr << "SurfaceGetCurrentTexture failed with status " << surfaceTexture.status;
        throw std::runtime_error("WGPU Error");
    }

    return surfaceTexture.texture;
}
