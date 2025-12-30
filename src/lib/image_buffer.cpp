#include "vanity/image_buffer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <utility>

namespace vanity {

// ImageBuffer implementation

ImageBuffer::ImageBuffer(int width, int height, int channels)
    : data_(new unsigned char[static_cast<size_t>(width) * height * channels])
    , width_(width)
    , height_(height)
    , channels_(channels) {
}

ImageBuffer::~ImageBuffer() {
    delete[] data_;
}

ImageBuffer::ImageBuffer(ImageBuffer&& other) noexcept
    : data_(other.data_)
    , width_(other.width_)
    , height_(other.height_)
    , channels_(other.channels_) {
    other.data_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
    other.channels_ = 0;
}

ImageBuffer& ImageBuffer::operator=(ImageBuffer&& other) noexcept {
    if (this != &other) {
        delete[] data_;

        data_ = other.data_;
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;

        other.data_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
        other.channels_ = 0;
    }
    return *this;
}

unsigned char* ImageBuffer::release() {
    unsigned char* ptr = data_;
    data_ = nullptr;
    width_ = 0;
    height_ = 0;
    channels_ = 0;
    return ptr;
}

// LoadedImage implementation

LoadedImage LoadedImage::load(const char* path, int& width, int& height, int& channels) {
    unsigned char* img = stbi_load(path, &width, &height, &channels, 0);
    return LoadedImage(img, width, height, channels);
}

LoadedImage::LoadedImage(unsigned char* data, int width, int height, int channels)
    : data_(data)
    , width_(width)
    , height_(height)
    , channels_(channels) {
}

LoadedImage::~LoadedImage() {
    if (data_) {
        stbi_image_free(data_);
    }
}

LoadedImage::LoadedImage(LoadedImage&& other) noexcept
    : data_(other.data_)
    , width_(other.width_)
    , height_(other.height_)
    , channels_(other.channels_) {
    other.data_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
    other.channels_ = 0;
}

LoadedImage& LoadedImage::operator=(LoadedImage&& other) noexcept {
    if (this != &other) {
        if (data_) {
            stbi_image_free(data_);
        }

        data_ = other.data_;
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;

        other.data_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
        other.channels_ = 0;
    }
    return *this;
}

unsigned char* LoadedImage::release() {
    unsigned char* ptr = data_;
    data_ = nullptr;
    width_ = 0;
    height_ = 0;
    channels_ = 0;
    return ptr;
}

} // namespace vanity
