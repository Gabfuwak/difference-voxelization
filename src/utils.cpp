#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

#include "utils.hpp"

namespace fs = std::filesystem;

namespace utils {

std::string readFile(const fs::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open file: " + path.string());
    };
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

fs::path findShaderPath(const fs::path& filename) {
    // Check if path is absolute
    if (filename.is_absolute()) {
        return filename;
    }

    // Look inside shaders folder
    auto shaderPath = SHADERS_DIR / filename;
    if (fs::exists(shaderPath)) {
        return shaderPath;
    }

    auto errorMessage = std::format("Couldn't find shader \"{}\"\n", filename.string());
    throw std::runtime_error(errorMessage);
}

fs::path findAssetPath(const fs::path& filename) {
    auto path = ASSETS_DIR / filename;
    if (fs::exists(path)) {
        return path;
    }
    auto errorMessage = std::format("Couldn't find asset \"{}\"\n", filename.string());
    throw std::runtime_error(errorMessage);
}

tinyobj::ObjReader loadObjFile(const fs::path& filename) {
    tinyobj::ObjReaderConfig config;
    tinyobj::ObjReader reader;

    auto path = findAssetPath(filename);
    if (!reader.ParseFromFile(path, config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjLoader: " << reader.Error() << '\n';
        }
        exit(1);
    }
    if (!reader.Warning().empty()) {
        std::cerr << "TinyObjLoader" << reader.Warning() << '\n';
    }

    return reader;
}

std::size_t sizeofArray(const auto& c) {
    using T = std::ranges::range_value_t<decltype(c)>;
    return c.size() * sizeof(T);
}

} // namespace utils
