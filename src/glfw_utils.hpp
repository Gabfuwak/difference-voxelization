#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

namespace utils {

wgpu::Extent2D getFramebufferSize(GLFWwindow* window);

} // namespace utils