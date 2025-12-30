#include <gtest/gtest.h>
#include "vanity/image_ops.hpp"
#include "vanity/image_buffer.hpp"
#include <cstring>

using namespace vanity;

TEST(ImageOpsTest, CalculateBorderedDimensions) {
    int out_w, out_h;
    calculate_bordered_dimensions(100, 100, 10, out_w, out_h);
    EXPECT_EQ(out_w, 120);
    EXPECT_EQ(out_h, 120);

    calculate_bordered_dimensions(50, 75, 5, out_w, out_h);
    EXPECT_EQ(out_w, 60);
    EXPECT_EQ(out_h, 85);

    calculate_bordered_dimensions(10, 20, 0, out_w, out_h);
    EXPECT_EQ(out_w, 10);
    EXPECT_EQ(out_h, 20);
}

TEST(ImageOpsTest, FillBuffer) {
    unsigned char buffer[100];
    std::memset(buffer, 0, 100);

    fill_buffer(buffer, 100, 255);

    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(buffer[i], 255);
    }
}

TEST(ImageOpsTest, AddBorderCreatesCorrectDimensions) {
    // Create simple 2x2 RGB image
    unsigned char src[2 * 2 * 3] = {
        255, 0, 0,    0, 255, 0,    // Red, Green
        0, 0, 255,    255, 255, 0   // Blue, Yellow
    };

    int border = 1;
    int new_w = 2 + 2 * border;
    int new_h = 2 + 2 * border;
    ImageBuffer dst(new_w, new_h, 3);

    unsigned char white[4] = {255, 255, 255, 255};
    bool result = add_border(src, 2, 2, 3, dst.get(), border, white);

    EXPECT_TRUE(result);

    // Check corner pixel is white (border)
    EXPECT_EQ(dst.get()[0], 255);
    EXPECT_EQ(dst.get()[1], 255);
    EXPECT_EQ(dst.get()[2], 255);

    // Check center pixel is original red pixel
    int center_idx = ((1) * new_w + (1)) * 3;  // (1,1) in bordered image
    EXPECT_EQ(dst.get()[center_idx + 0], 255);  // Red
    EXPECT_EQ(dst.get()[center_idx + 1], 0);
    EXPECT_EQ(dst.get()[center_idx + 2], 0);

    // Check original green pixel (was at 1,0 -> now at 2,1)
    int green_idx = ((1) * new_w + (2)) * 3;
    EXPECT_EQ(dst.get()[green_idx + 0], 0);
    EXPECT_EQ(dst.get()[green_idx + 1], 255);  // Green
    EXPECT_EQ(dst.get()[green_idx + 2], 0);
}

TEST(ImageOpsTest, AddBorderWithLargerBorder) {
    // 1x1 grayscale image
    unsigned char src[1] = {128};

    int border = 3;
    int new_w = 1 + 2 * border;
    int new_h = 1 + 2 * border;
    ImageBuffer dst(new_w, new_h, 1);

    unsigned char black[4] = {0, 0, 0, 0};
    bool result = add_border(src, 1, 1, 1, dst.get(), border, black);

    EXPECT_TRUE(result);

    // Check center pixel is original value
    int center_idx = (3 * new_w + 3);
    EXPECT_EQ(dst.get()[center_idx], 128);

    // Check border pixels are black
    EXPECT_EQ(dst.get()[0], 0);  // Top-left corner
    EXPECT_EQ(dst.get()[new_w - 1], 0);  // Top-right corner
    EXPECT_EQ(dst.get()[(new_h - 1) * new_w], 0);  // Bottom-left corner
}

TEST(ImageOpsTest, AddBorderRejectsInvalidParams) {
    unsigned char src[4] = {0};
    unsigned char dst[16] = {0};
    unsigned char color[4] = {255, 255, 255, 255};

    // Negative border width
    EXPECT_FALSE(add_border(src, 1, 1, 1, dst, -1, color));

    // Null source pointer
    EXPECT_FALSE(add_border(nullptr, 1, 1, 1, dst, 1, color));

    // Null destination pointer
    EXPECT_FALSE(add_border(src, 1, 1, 1, nullptr, 1, color));

    // Invalid dimensions
    EXPECT_FALSE(add_border(src, 0, 1, 1, dst, 1, color));
    EXPECT_FALSE(add_border(src, 1, 0, 1, dst, 1, color));
    EXPECT_FALSE(add_border(src, 1, 1, 0, dst, 1, color));
}

TEST(ImageOpsTest, AddBorderWithZeroBorder) {
    unsigned char src[2 * 2 * 3] = {
        255, 0, 0,    0, 255, 0,
        0, 0, 255,    255, 255, 255
    };

    ImageBuffer dst(2, 2, 3);
    unsigned char white[4] = {255, 255, 255, 255};

    bool result = add_border(src, 2, 2, 3, dst.get(), 0, white);

    EXPECT_TRUE(result);

    // Should be identical to source
    for (size_t i = 0; i < 12; i++) {
        EXPECT_EQ(dst.get()[i], src[i]);
    }
}
