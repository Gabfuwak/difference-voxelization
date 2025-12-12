#pragma once

#include <filesystem>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>
#include <tiny_obj_loader.h>

namespace utils {

std::string readFile(const std::filesystem::path& path);

std::filesystem::path findShaderPath(const std::filesystem::path& filename);

std::filesystem::path findAssetPath(const std::filesystem::path& filename);

tinyobj::ObjReader loadObjFile(const std::filesystem::path& file);

size_t sizeofArray(const auto& c);

} // namespace utils
