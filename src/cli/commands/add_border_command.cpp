#include "../command_registry.hpp"
#include "vanity/image_buffer.hpp"
#include "vanity/image_ops.hpp"
#include "vanity/image_io.hpp"
#include "stb_image.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

namespace vanity {

class AddBorderCommand : public Command {
private:
    bool is_supported_image_file(const std::filesystem::path& path) const {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
    }

    CommandResult process_single_file(const char* input_path, const char* output_path, int border_width) {
        // Load image
        int width, height, channels;
        LoadedImage img = LoadedImage::load(input_path, width, height, channels);

        if (!img.get()) {
            std::string error = "Error: Failed to load image '";
            error += input_path;
            error += "'\nReason: ";
            error += stbi_failure_reason();
            return {1, error};
        }

        std::cout << "Loaded image: " << width << "x" << height
                  << " with " << channels << " channels\n";

        // Calculate new dimensions
        int new_width, new_height;
        calculate_bordered_dimensions(width, height, border_width, new_width, new_height);

        // Create output buffer
        ImageBuffer output(new_width, new_height, channels);

        // Add white border
        unsigned char white[4] = {255, 255, 255, 255};
        if (!add_border(img.get(), width, height, channels, output.get(), border_width, white)) {
            return {1, "Error: Failed to add border"};
        }

        // Write output image
        if (!write_image(output_path, new_width, new_height, channels, output.get())) {
            return {1, "Error: Failed to write image"};
        }

        std::cout << "Successfully wrote image: " << new_width << "x" << new_height
                  << " to '" << output_path << "'\n";

        return {0, ""};
    }

public:
    CommandResult execute(int argc, char* argv[]) override {
        namespace fs = std::filesystem;

        // Support two modes:
        // 1. File mode: vanity border <input_image> <output_image> <border_width>
        // 2. Directory mode: vanity border <directory> <border_width>
        if (argc != 3 && argc != 4) {
            print_usage(argv[0]);
            return {1, ""};
        }

        // Check if this is directory mode (2 arguments) or file mode (3 arguments)
        if (argc == 3) {
            // Directory mode
            const char* dir_path = argv[1];
            int border_width = std::atoi(argv[2]);

            if (border_width <= 0) {
                return {1, "Error: Border width must be a positive integer"};
            }

            fs::path directory(dir_path);
            if (!fs::exists(directory)) {
                return {1, "Error: Directory does not exist"};
            }

            if (!fs::is_directory(directory)) {
                return {1, "Error: Path is not a directory"};
            }

            // Collect all supported image files
            std::vector<fs::path> image_files;
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file() && is_supported_image_file(entry.path())) {
                    image_files.push_back(entry.path());
                }
            }

            if (image_files.empty()) {
                return {1, "Error: No JPEG or PNG files found in directory"};
            }

            std::cout << "Found " << image_files.size() << " image file(s) to process\n";

            // Process each image file
            int success_count = 0;
            int failure_count = 0;
            for (const auto& input_file : image_files) {
                // Generate output filename: original_name_vanity_<border-size>.ext
                std::string stem = input_file.stem().string();
                std::string extension = input_file.extension().string();
                std::string output_filename = stem + "_vanity_" + std::to_string(border_width) + extension;
                fs::path output_file = input_file.parent_path() / output_filename;

                std::cout << "\nProcessing: " << input_file.filename().string() << " -> " << output_filename << "\n";

                CommandResult result = process_single_file(
                    input_file.string().c_str(),
                    output_file.string().c_str(),
                    border_width
                );

                if (result.exit_code == 0) {
                    success_count++;
                } else {
                    failure_count++;
                    std::cerr << result.message << "\n";
                }
            }

            std::cout << "\nCompleted: " << success_count << " successful, " << failure_count << " failed\n";
            return {failure_count > 0 ? 1 : 0, ""};

        } else {
            // File mode (original behavior)
            const char* input_path = argv[1];
            const char* output_path = argv[2];
            int border_width = std::atoi(argv[3]);

            if (border_width <= 0) {
                return {1, "Error: Border width must be a positive integer"};
            }

            return process_single_file(input_path, output_path, border_width);
        }
    }

    void print_usage(const char* program_name) const override {
        std::cout << "Usage:\n";
        std::cout << "  " << program_name << " <input_image> <output_image> <border_width>\n";
        std::cout << "  " << program_name << " <directory> <border_width>\n\n";
        std::cout << "File mode:\n";
        std::cout << "  input_image:  Path to the input image file\n";
        std::cout << "  output_image: Path to save the output image\n";
        std::cout << "  border_width: Width of the white border in pixels\n\n";
        std::cout << "Directory mode:\n";
        std::cout << "  directory:    Path to directory containing images\n";
        std::cout << "  border_width: Width of the white border in pixels\n";
        std::cout << "                (processes all JPEG and PNG files, saves as filename_vanity_<border_width>.ext)\n";
    }

    const char* name() const override {
        return "border";
    }

    const char* description() const override {
        return "Add white border around an image or all images in a directory";
    }
};

// Factory function to create the command (called from commands.cpp)
std::unique_ptr<Command> create_add_border_command() {
    return std::make_unique<AddBorderCommand>();
}

} // namespace vanity
