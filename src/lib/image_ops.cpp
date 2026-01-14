#include "vanity/image_ops.hpp"
#include <cmath>
#include <cstring>

namespace vanity {

void calculate_bordered_dimensions(int src_width, int src_height, int border_width,
                                   int& out_width, int& out_height) {
    out_width = src_width + 2 * border_width;
    out_height = src_height + 2 * border_width;
}

void calculate_bordered_dimensions(int src_width, int src_height,
                                   int border_h, int border_v,
                                   int& out_width, int& out_height) {
    out_width = src_width + 2 * border_h;
    out_height = src_height + 2 * border_v;
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

bool add_border(const unsigned char* src, int src_width, int src_height, int channels,
                unsigned char* dst, int border_h, int border_v, const unsigned char border_color[4]) {
    // Validate parameters
    if (!src || !dst || border_h < 0 || border_v < 0 || src_width <= 0 || src_height <= 0 || channels <= 0) {
        return false;
    }

    // Calculate new dimensions
    int new_width = src_width + 2 * border_h;
    int new_height = src_height + 2 * border_v;

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
            int dst_idx = ((y + border_v) * new_width + (x + border_h)) * channels;

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

void calculate_average_color(const unsigned char* src, int width, int height, int channels,
                              unsigned char avg_color[4]) {
    if (!src || width <= 0 || height <= 0 || channels <= 0) {
        // Default to white if invalid
        avg_color[0] = avg_color[1] = avg_color[2] = avg_color[3] = 255;
        return;
    }

    size_t total_pixels = static_cast<size_t>(width) * height;
    unsigned long long sum[4] = {0, 0, 0, 0};

    // Sum up all pixel values
    for (size_t i = 0; i < total_pixels; i++) {
        for (int c = 0; c < channels && c < 4; c++) {
            sum[c] += src[i * channels + c];
        }
    }

    // Calculate average
    for (int c = 0; c < channels && c < 4; c++) {
        avg_color[c] = static_cast<unsigned char>(sum[c] / total_pixels);
    }

    // Fill remaining channels with default values
    for (int c = channels; c < 4; c++) {
        avg_color[c] = (c == 3) ? 255 : 0; // Alpha = 255, others = 0
    }
}

bool add_gradient_border(const unsigned char* src, int src_width, int src_height, int channels,
                         unsigned char* dst, int border_width, const unsigned char start_color[4]) {
    // Validate parameters
    if (!src || !dst || border_width < 0 || src_width <= 0 || src_height <= 0 || channels <= 0) {
        return false;
    }

    // Calculate new dimensions
    int new_width = src_width + 2 * border_width;
    int new_height = src_height + 2 * border_width;

    // White color (gradient end)
    unsigned char end_color[4] = {255, 255, 255, 255};

    // Calculate maximum diagonal distance for normalization
    float max_diagonal = std::sqrt(static_cast<float>(new_width * new_width + new_height * new_height));

    // Fill border with gradient
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            int dst_idx = (y * new_width + x) * channels;

            // Check if this pixel is in the border region
            bool in_border = (x < border_width || x >= src_width + border_width ||
                             y < border_width || y >= src_height + border_width);

            if (in_border) {
                // Calculate diagonal distance from top-left corner
                float diag_dist = std::sqrt(static_cast<float>(x * x + y * y));

                // Normalize to 0.0-1.0 range (0 at top-left, 1 at bottom-right)
                float factor = diag_dist / max_diagonal;

                // Interpolate between start_color and white
                for (int c = 0; c < channels; c++) {
                    float value = start_color[c] * (1.0f - factor) + end_color[c] * factor;
                    dst[dst_idx + c] = static_cast<unsigned char>(value + 0.5f);
                }
            } else {
                // Copy source pixel
                int src_x = x - border_width;
                int src_y = y - border_width;
                int src_idx = (src_y * src_width + src_x) * channels;

                for (int c = 0; c < channels; c++) {
                    dst[dst_idx + c] = src[src_idx + c];
                }
            }
        }
    }

    return true;
}

bool add_gradient_border(const unsigned char* src, int src_width, int src_height, int channels,
                         unsigned char* dst, int border_h, int border_v, const unsigned char start_color[4]) {
    // Validate parameters
    if (!src || !dst || border_h < 0 || border_v < 0 || src_width <= 0 || src_height <= 0 || channels <= 0) {
        return false;
    }

    // Calculate new dimensions
    int new_width = src_width + 2 * border_h;
    int new_height = src_height + 2 * border_v;

    // White color (gradient end)
    unsigned char end_color[4] = {255, 255, 255, 255};

    // Calculate maximum diagonal distance for normalization
    float max_diagonal = std::sqrt(static_cast<float>(new_width * new_width + new_height * new_height));

    // Fill border with gradient
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            int dst_idx = (y * new_width + x) * channels;

            // Check if this pixel is in the border region
            bool in_border = (x < border_h || x >= src_width + border_h ||
                             y < border_v || y >= src_height + border_v);

            if (in_border) {
                // Calculate diagonal distance from top-left corner
                float diag_dist = std::sqrt(static_cast<float>(x * x + y * y));

                // Normalize to 0.0-1.0 range (0 at top-left, 1 at bottom-right)
                float factor = diag_dist / max_diagonal;

                // Interpolate between start_color and white
                for (int c = 0; c < channels; c++) {
                    float value = start_color[c] * (1.0f - factor) + end_color[c] * factor;
                    dst[dst_idx + c] = static_cast<unsigned char>(value + 0.5f);
                }
            } else {
                // Copy source pixel
                int src_x = x - border_h;
                int src_y = y - border_v;
                int src_idx = (src_y * src_width + src_x) * channels;

                for (int c = 0; c < channels; c++) {
                    dst[dst_idx + c] = src[src_idx + c];
                }
            }
        }
    }

    return true;
}

} // namespace vanity
