#ifndef VANITY_IMAGE_BUFFER_HPP
#define VANITY_IMAGE_BUFFER_HPP

#include <cstddef>

namespace vanity {

// RAII wrapper for allocated image buffers (using new[]/delete[])
class ImageBuffer {
public:
    // Constructor: allocates buffer
    ImageBuffer(int width, int height, int channels);

    // Destructor: automatically frees buffer
    ~ImageBuffer();

    // Move constructor: transfer ownership
    ImageBuffer(ImageBuffer&& other) noexcept;

    // Move assignment: transfer ownership
    ImageBuffer& operator=(ImageBuffer&& other) noexcept;

    // Delete copy constructor and assignment (prevent double-free)
    ImageBuffer(const ImageBuffer&) = delete;
    ImageBuffer& operator=(const ImageBuffer&) = delete;

    // Accessors
    unsigned char* get() { return data_; }
    const unsigned char* get() const { return data_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int channels() const { return channels_; }
    size_t byte_size() const { return static_cast<size_t>(width_) * height_ * channels_; }

    // Release ownership (caller becomes responsible for delete[])
    unsigned char* release();

private:
    unsigned char* data_;
    int width_;
    int height_;
    int channels_;
};

// RAII wrapper for stb-loaded images (using stbi_image_free)
class LoadedImage {
public:
    // Factory method: loads image from file
    static LoadedImage load(const char* path, int& width, int& height, int& channels);

    // Constructor: takes ownership of stb-loaded data
    LoadedImage(unsigned char* data, int width, int height, int channels);

    // Destructor: automatically frees buffer with stbi_image_free
    ~LoadedImage();

    // Move constructor: transfer ownership
    LoadedImage(LoadedImage&& other) noexcept;

    // Move assignment: transfer ownership
    LoadedImage& operator=(LoadedImage&& other) noexcept;

    // Delete copy constructor and assignment (prevent double-free)
    LoadedImage(const LoadedImage&) = delete;
    LoadedImage& operator=(const LoadedImage&) = delete;

    // Accessors
    unsigned char* get() { return data_; }
    const unsigned char* get() const { return data_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int channels() const { return channels_; }
    size_t byte_size() const { return static_cast<size_t>(width_) * height_ * channels_; }

    // Release ownership (caller becomes responsible for stbi_image_free)
    unsigned char* release();

private:
    unsigned char* data_;
    int width_;
    int height_;
    int channels_;
};

} // namespace vanity

#endif // VANITY_IMAGE_BUFFER_HPP
