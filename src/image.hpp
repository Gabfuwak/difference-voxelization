#pragma once

#include <stb_image.h>
#include <utils.hpp>

class Image {
public:
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> data;
    int width, height;
    int channels;

    Image(const char* filename, int desired_channels): data(nullptr, stbi_image_free) {
        if (auto* ptr = stbi_load(filename, &width, &height, &channels, desired_channels)) {
            data.reset(ptr);
        } else {
            throw std::runtime_error("stbi_load failed");
        }
    }

    static Image load(const std::filesystem::path& filename, int desired_channels = 4) {
        auto assetPath = utils::findAssetPath(filename);
        return Image(assetPath.c_str(), desired_channels);
    }

    uint32_t getWidth() {
        return static_cast<uint32_t>(width);
    }

    uint32_t getHeight() {
        return static_cast<uint32_t>(height);
    }
};
