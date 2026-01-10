#ifndef VANITY_IMAGE_OPS_HPP
#define VANITY_IMAGE_OPS_HPP

#include <cstddef>

namespace vanity
{
  // Calculate dimensions for bordered image
  void calculate_bordered_dimensions(int src_width, int src_height, int border_width,
                                     int &out_width, int &out_height);

  // Fill buffer with a single value
  void fill_buffer(unsigned char *buffer, size_t size, unsigned char value);

  // Add border around image
  // src: source image buffer
  // src_width, src_height, channels: source image dimensions
  // dst: destination buffer (must be pre-allocated to correct size)
  // border_width: width of border in pixels
  // border_color: RGBA color for border (only uses channels needed)
  // Returns: true on success, false on invalid parameters
  bool add_border(const unsigned char *src, int src_width, int src_height, int channels,
                  unsigned char *dst, int border_width, const unsigned char border_color[4]);

  // Resize image using bilinear interpolation
  // src: source image buffer
  // src_width, src_height, channels: source image dimensions
  // dst: destination buffer (must be pre-allocated to dst_width * dst_height * channels)
  // dst_width, dst_height: target dimensions
  // Returns: true on success, false on invalid parameters
  bool resize_image(const unsigned char *src, int src_width, int src_height, int channels,
                    unsigned char *dst, int dst_width, int dst_height);

  // Add padding around image (centers source in larger destination)
  // src: source image buffer
  // src_width, src_height, channels: source image dimensions
  // dst: destination buffer (must be pre-allocated to dst_width * dst_height * channels)
  // dst_width, dst_height: target dimensions (must be >= source dimensions)
  // padding_color: RGBA color for padding (only uses channels needed)
  // Returns: true on success, false on invalid parameters
  bool add_padding(const unsigned char *src, int src_width, int src_height, int channels,
                   unsigned char *dst, int dst_width, int dst_height,
                   const unsigned char padding_color[4]);

} // namespace vanity

#endif // VANITY_IMAGE_OPS_HPP
