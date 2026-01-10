#include "vanity/image_ops.hpp"
#include <cmath>
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

// Bilinear interpolation for upscaling
static void resize_bilinear(const unsigned char* src, int src_width, int src_height, int channels,
                            unsigned char* dst, int dst_width, int dst_height) {
    float x_ratio = static_cast<float>(src_width) / dst_width;
    float y_ratio = static_cast<float>(src_height) / dst_height;

    for (int y = 0; y < dst_height; y++) {
        for (int x = 0; x < dst_width; x++) {
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;

            int x0 = static_cast<int>(src_x);
            int y0 = static_cast<int>(src_y);
            int x1 = (x0 + 1 < src_width) ? x0 + 1 : x0;
            int y1 = (y0 + 1 < src_height) ? y0 + 1 : y0;

            float x_weight = src_x - x0;
            float y_weight = src_y - y0;

            for (int c = 0; c < channels; c++) {
                unsigned char p00 = src[(y0 * src_width + x0) * channels + c];
                unsigned char p10 = src[(y0 * src_width + x1) * channels + c];
                unsigned char p01 = src[(y1 * src_width + x0) * channels + c];
                unsigned char p11 = src[(y1 * src_width + x1) * channels + c];

                float top = p00 * (1 - x_weight) + p10 * x_weight;
                float bottom = p01 * (1 - x_weight) + p11 * x_weight;
                float value = top * (1 - y_weight) + bottom * y_weight;

                dst[(y * dst_width + x) * channels + c] = static_cast<unsigned char>(value);
            }
        }
    }
}

// Area averaging for downscaling - averages all source pixels that map to each destination pixel
static void resize_area_average(const unsigned char* src, int src_width, int src_height, int channels,
                                unsigned char* dst, int dst_width, int dst_height) {
    float x_ratio = static_cast<float>(src_width) / dst_width;
    float y_ratio = static_cast<float>(src_height) / dst_height;

    for (int y = 0; y < dst_height; y++) {
        for (int x = 0; x < dst_width; x++) {
            // Calculate the source region that maps to this destination pixel
            float src_x_start = x * x_ratio;
            float src_x_end = (x + 1) * x_ratio;
            float src_y_start = y * y_ratio;
            float src_y_end = (y + 1) * y_ratio;

            // Clamp to source bounds
            int x_start = static_cast<int>(src_x_start);
            int x_end = static_cast<int>(std::ceil(src_x_end));
            int y_start = static_cast<int>(src_y_start);
            int y_end = static_cast<int>(std::ceil(src_y_end));

            if (x_end > src_width) x_end = src_width;
            if (y_end > src_height) y_end = src_height;

            for (int c = 0; c < channels; c++) {
                float sum = 0.0f;
                float weight_sum = 0.0f;

                // Iterate over all source pixels in the region with proper weighting
                for (int sy = y_start; sy < y_end; sy++) {
                    // Calculate vertical weight (how much of this row falls in the region)
                    float y_weight = 1.0f;
                    if (sy < src_y_start) {
                        y_weight = 1.0f - (src_y_start - sy);
                    } else if (sy + 1 > src_y_end) {
                        y_weight = src_y_end - sy;
                    }
                    if (y_weight <= 0) continue;

                    for (int sx = x_start; sx < x_end; sx++) {
                        // Calculate horizontal weight
                        float x_weight = 1.0f;
                        if (sx < src_x_start) {
                            x_weight = 1.0f - (src_x_start - sx);
                        } else if (sx + 1 > src_x_end) {
                            x_weight = src_x_end - sx;
                        }
                        if (x_weight <= 0) continue;

                        float pixel_weight = x_weight * y_weight;
                        sum += src[(sy * src_width + sx) * channels + c] * pixel_weight;
                        weight_sum += pixel_weight;
                    }
                }

                float value = (weight_sum > 0) ? (sum / weight_sum) : 0;
                // Clamp to valid range
                if (value < 0) value = 0;
                if (value > 255) value = 255;
                dst[(y * dst_width + x) * channels + c] = static_cast<unsigned char>(value + 0.5f);
            }
        }
    }
}

bool resize_image(const unsigned char* src, int src_width, int src_height, int channels,
                  unsigned char* dst, int dst_width, int dst_height) {
    // Validate parameters
    if (!src || !dst || src_width <= 0 || src_height <= 0 ||
        dst_width <= 0 || dst_height <= 0 || channels <= 0) {
        return false;
    }

    // Use area averaging for downscaling (better quality, no aliasing)
    // Use bilinear interpolation for upscaling
    bool is_downscaling = (dst_width < src_width) || (dst_height < src_height);

    if (is_downscaling) {
        resize_area_average(src, src_width, src_height, channels, dst, dst_width, dst_height);
    } else {
        resize_bilinear(src, src_width, src_height, channels, dst, dst_width, dst_height);
    }

    return true;
}

bool add_padding(const unsigned char* src, int src_width, int src_height, int channels,
                 unsigned char* dst, int dst_width, int dst_height,
                 const unsigned char padding_color[4]) {
    // Validate parameters
    if (!src || !dst || src_width <= 0 || src_height <= 0 ||
        dst_width < src_width || dst_height < src_height || channels <= 0) {
        return false;
    }

    // Calculate padding on each side (center the source image)
    int pad_left = (dst_width - src_width) / 2;
    int pad_top = (dst_height - src_height) / 2;

    // Fill destination buffer with padding color
    size_t dst_size = static_cast<size_t>(dst_width) * dst_height * channels;
    for (size_t i = 0; i < dst_size; i += channels) {
        for (int c = 0; c < channels; c++) {
            dst[i + c] = padding_color[c];
        }
    }

    // Copy source image to center of destination
    for (int y = 0; y < src_height; y++) {
        for (int x = 0; x < src_width; x++) {
            int src_idx = (y * src_width + x) * channels;
            int dst_idx = ((y + pad_top) * dst_width + (x + pad_left)) * channels;

            for (int c = 0; c < channels; c++) {
                dst[dst_idx + c] = src[src_idx + c];
            }
        }
    }

    return true;
}

} // namespace vanity
