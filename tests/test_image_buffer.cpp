#include <gtest/gtest.h>
#include "vanity/image_buffer.hpp"

using namespace vanity;

// Test ImageBuffer RAII behavior
TEST(ImageBufferTest, ConstructorAllocatesMemory) {
    ImageBuffer buf(100, 100, 3);
    EXPECT_NE(buf.get(), nullptr);
    EXPECT_EQ(buf.width(), 100);
    EXPECT_EQ(buf.height(), 100);
    EXPECT_EQ(buf.channels(), 3);
    EXPECT_EQ(buf.byte_size(), 100 * 100 * 3);
}

TEST(ImageBufferTest, MoveConstructorTransfersOwnership) {
    ImageBuffer buf1(50, 50, 4);
    unsigned char* ptr = buf1.get();

    ImageBuffer buf2(std::move(buf1));
    EXPECT_EQ(buf2.get(), ptr);
    EXPECT_EQ(buf1.get(), nullptr);
    EXPECT_EQ(buf1.width(), 0);
    EXPECT_EQ(buf1.height(), 0);
    EXPECT_EQ(buf1.channels(), 0);
}

TEST(ImageBufferTest, MoveAssignmentTransfersOwnership) {
    ImageBuffer buf1(30, 30, 3);
    unsigned char* ptr = buf1.get();

    ImageBuffer buf2(10, 10, 1);
    buf2 = std::move(buf1);

    EXPECT_EQ(buf2.get(), ptr);
    EXPECT_EQ(buf1.get(), nullptr);
}

TEST(ImageBufferTest, ReleaseTransfersOwnership) {
    ImageBuffer buf(10, 10, 1);
    unsigned char* ptr = buf.release();

    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(buf.get(), nullptr);
    EXPECT_EQ(buf.width(), 0);
    EXPECT_EQ(buf.height(), 0);
    EXPECT_EQ(buf.channels(), 0);

    // Manual cleanup after release
    delete[] ptr;
}

// Test LoadedImage separately
TEST(LoadedImageTest, InvalidPathReturnsNull) {
    int w, h, c;
    LoadedImage img = LoadedImage::load("nonexistent_file_xyz.png", w, h, c);
    EXPECT_EQ(img.get(), nullptr);
}

TEST(LoadedImageTest, MoveConstructorTransfersOwnership) {
    // Create a LoadedImage with nullptr (simulating failed load)
    LoadedImage img1(nullptr, 0, 0, 0);
    LoadedImage img2(std::move(img1));

    EXPECT_EQ(img2.get(), nullptr);
    EXPECT_EQ(img1.get(), nullptr);
}
