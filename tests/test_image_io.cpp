#include <gtest/gtest.h>
#include "vanity/image_io.hpp"
#include <fstream>
#include <cstdio>

using namespace vanity;

TEST(ImageIOTest, DetectFormatPNG) {
    EXPECT_EQ(detect_format("image.png"), ImageFormat::PNG);
    EXPECT_EQ(detect_format("image.PNG"), ImageFormat::PNG);
    EXPECT_EQ(detect_format("/path/to/image.png"), ImageFormat::PNG);
    EXPECT_EQ(detect_format("../relative/path/image.PNG"), ImageFormat::PNG);
}

TEST(ImageIOTest, DetectFormatJPG) {
    EXPECT_EQ(detect_format("image.jpg"), ImageFormat::JPG);
    EXPECT_EQ(detect_format("image.jpeg"), ImageFormat::JPG);
    EXPECT_EQ(detect_format("image.JPG"), ImageFormat::JPG);
    EXPECT_EQ(detect_format("image.JPEG"), ImageFormat::JPG);
    EXPECT_EQ(detect_format("/path/image.jpg"), ImageFormat::JPG);
}

TEST(ImageIOTest, DetectFormatBMP) {
    EXPECT_EQ(detect_format("image.bmp"), ImageFormat::BMP);
    EXPECT_EQ(detect_format("image.BMP"), ImageFormat::BMP);
    EXPECT_EQ(detect_format("/path/to/image.bmp"), ImageFormat::BMP);
}

TEST(ImageIOTest, DetectFormatUnknown) {
    EXPECT_EQ(detect_format("image.tiff"), ImageFormat::UNKNOWN);
    EXPECT_EQ(detect_format("image.txt"), ImageFormat::UNKNOWN);
    EXPECT_EQ(detect_format("noextension"), ImageFormat::UNKNOWN);
    EXPECT_EQ(detect_format("file.gif"), ImageFormat::UNKNOWN);
}

// Integration test with actual file I/O
class ImageIOIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_image_png = "/tmp/test_vanity_image.png";
        test_image_jpg = "/tmp/test_vanity_image.jpg";
        test_image_bmp = "/tmp/test_vanity_image.bmp";
    }

    void TearDown() override {
        std::remove(test_image_png.c_str());
        std::remove(test_image_jpg.c_str());
        std::remove(test_image_bmp.c_str());
    }

    std::string test_image_png;
    std::string test_image_jpg;
    std::string test_image_bmp;
};

TEST_F(ImageIOIntegrationTest, WriteImagePNG) {
    // Create simple 2x2 RGB image
    unsigned char data[2 * 2 * 3] = {
        255, 0, 0,    0, 255, 0,
        0, 0, 255,    255, 255, 255
    };

    bool write_success = write_image(test_image_png.c_str(), 2, 2, 3, data);
    EXPECT_TRUE(write_success);

    // Verify file exists
    std::ifstream file(test_image_png);
    EXPECT_TRUE(file.good());
}

TEST_F(ImageIOIntegrationTest, WriteImageJPG) {
    unsigned char data[3 * 3 * 3] = {
        255, 0, 0,    0, 255, 0,    0, 0, 255,
        255, 255, 0,  255, 0, 255,  0, 255, 255,
        128, 128, 128,  64, 64, 64,  192, 192, 192
    };

    bool write_success = write_image(test_image_jpg.c_str(), 3, 3, 3, data, 90);
    EXPECT_TRUE(write_success);

    // Verify file exists
    std::ifstream file(test_image_jpg);
    EXPECT_TRUE(file.good());
}

TEST_F(ImageIOIntegrationTest, WriteImageBMP) {
    // Simple 1x1 grayscale image
    unsigned char data[1] = {128};

    bool write_success = write_image(test_image_bmp.c_str(), 1, 1, 1, data);
    EXPECT_TRUE(write_success);

    // Verify file exists
    std::ifstream file(test_image_bmp);
    EXPECT_TRUE(file.good());
}

TEST_F(ImageIOIntegrationTest, WriteImageUnknownFormat) {
    unsigned char data[4] = {0};

    bool write_success = write_image("/tmp/test.tiff", 1, 1, 1, data);
    EXPECT_FALSE(write_success);
}
