#include "glfw_utils.hpp"

namespace utils {

wgpu::Extent2D getFramebufferSize(GLFWwindow* window) {
    int fWidth, fHeight;
    glfwGetFramebufferSize(window, &fWidth, &fHeight);
    return wgpu::Extent2D {
        static_cast<uint32_t>(fWidth),
        static_cast<uint32_t>(fHeight),
    };
}

} // namespace utils
