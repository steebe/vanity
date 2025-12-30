#ifndef VANITY_IMAGE_IO_HPP
#define VANITY_IMAGE_IO_HPP

#include <string>

namespace vanity {

// Supported image output formats
enum class ImageFormat {
    PNG,
    JPG,
    BMP,
    UNKNOWN
};

// Detect image format from file extension
ImageFormat detect_format(const std::string& path);

// Write image to file with format auto-detection
// quality: JPEG quality (0-100), ignored for PNG/BMP
bool write_image(const char* path, int width, int height, int channels,
                 const unsigned char* data, int quality = 95);

} // namespace vanity

#endif // VANITY_IMAGE_IO_HPP
