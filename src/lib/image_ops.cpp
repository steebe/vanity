#include "vanity/image_ops.hpp"
#include <cstring>

namespace vanity {

void calculate_bordered_dimensions(int src_width, int src_height, int border_width,
                                   int& out_width, int& out_height) {
    out_width = src_width + 2 * border_width;
    out_height = src_height + 2 * border_width;
}

void fill_buffer(unsigned char* buffer, size_t size, unsigned char value) {
    std::memset(buffer, value, size);
}

bool add_border(const unsigned char* src, int src_width, int src_height, int channels,
                unsigned char* dst, int border_width, const unsigned char border_color[4]) {
    // Validate parameters
    if (!src || !dst || border_width < 0 || src_width <= 0 || src_height <= 0 || channels <= 0) {
        return false;
    }

    // Calculate new dimensions
    int new_width = src_width + 2 * border_width;
    int new_height = src_height + 2 * border_width;

    // Fill destination buffer with border color
    size_t new_size = static_cast<size_t>(new_width) * new_height * channels;
    for (size_t i = 0; i < new_size; i += channels) {
        for (int c = 0; c < channels; c++) {
            dst[i + c] = border_color[c];
        }
    }

    // Copy original image to center of new image
    for (int y = 0; y < src_height; y++) {
        for (int x = 0; x < src_width; x++) {
            int src_idx = (y * src_width + x) * channels;
            int dst_idx = ((y + border_width) * new_width + (x + border_width)) * channels;

            for (int c = 0; c < channels; c++) {
                dst[dst_idx + c] = src[src_idx + c];
            }
        }
    }

    return true;
}

} // namespace vanity
