#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "vanity/image_io.hpp"
#include <algorithm>
#include <cctype>

namespace vanity {

ImageFormat detect_format(const std::string& path) {
    // Convert to lowercase for case-insensitive comparison
    std::string lower_path = path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower_path.ends_with(".png")) {
        return ImageFormat::PNG;
    } else if (lower_path.ends_with(".jpg") || lower_path.ends_with(".jpeg")) {
        return ImageFormat::JPG;
    } else if (lower_path.ends_with(".bmp")) {
        return ImageFormat::BMP;
    }

    return ImageFormat::UNKNOWN;
}

bool write_image(const char* path, int width, int height, int channels,
                 const unsigned char* data, int quality) {
    ImageFormat format = detect_format(path);

    switch (format) {
        case ImageFormat::PNG:
            return stbi_write_png(path, width, height, channels, data, width * channels) != 0;

        case ImageFormat::JPG:
            return stbi_write_jpg(path, width, height, channels, data, quality) != 0;

        case ImageFormat::BMP:
            return stbi_write_bmp(path, width, height, channels, data) != 0;

        case ImageFormat::UNKNOWN:
            return false;
    }

    return false;
}

} // namespace vanity
