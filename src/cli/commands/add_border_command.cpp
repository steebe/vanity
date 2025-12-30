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
#include <optional>

namespace vanity {

class AddBorderCommand : public Command {
private:
    bool is_supported_image_file(const std::filesystem::path& path) const {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
    }

    CommandResult process_single_file(const char* input_path, const char* output_path, int border_width, bool inner_border) {
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

        // If inner border is requested, first add a 10px black border
        unsigned char* current_data = img.get();
        int current_width = width;
        int current_height = height;
        std::optional<ImageBuffer> inner_buffer;

        if (inner_border) {
            const int inner_border_width = 10;
            int inner_width, inner_height;
            calculate_bordered_dimensions(width, height, inner_border_width, inner_width, inner_height);

            inner_buffer.emplace(inner_width, inner_height, channels);
            unsigned char black[4] = {0, 0, 0, 255};

            if (!add_border(img.get(), width, height, channels, inner_buffer->get(), inner_border_width, black)) {
                return {1, "Error: Failed to add inner border"};
            }

            current_data = inner_buffer->get();
            current_width = inner_width;
            current_height = inner_height;
            std::cout << "Added 10px black inner border\n";
        }

        // Calculate new dimensions for white border
        int new_width, new_height;
        calculate_bordered_dimensions(current_width, current_height, border_width, new_width, new_height);

        // Create output buffer
        ImageBuffer output(new_width, new_height, channels);

        // Add white border
        unsigned char white[4] = {255, 255, 255, 255};
        if (!add_border(current_data, current_width, current_height, channels, output.get(), border_width, white)) {
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

        // Check for --inner flag
        bool inner_border = false;
        std::vector<std::string> args;
        for (int i = 1; i < argc; i++) {
            if (std::string(argv[i]) == "--inner") {
                inner_border = true;
            } else {
                args.push_back(argv[i]);
            }
        }

        // Support two modes:
        // 1. File mode: vanity border <input_image> <output_image> <border_width> [--inner]
        // 2. Directory mode: vanity border <directory> <border_width> [--inner]
        if (args.size() != 2 && args.size() != 3) {
            print_usage(argv[0]);
            return {1, ""};
        }

        // Check if this is directory mode (2 arguments) or file mode (3 arguments)
        if (args.size() == 2) {
            // Directory mode
            const char* dir_path = args[0].c_str();
            int border_width = std::atoi(args[1].c_str());

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
            if (inner_border) {
                std::cout << "Inner border mode enabled (10px black border)\n";
            }

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
                    border_width,
                    inner_border
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
            const char* input_path = args[0].c_str();
            const char* output_path = args[1].c_str();
            int border_width = std::atoi(args[2].c_str());

            if (border_width <= 0) {
                return {1, "Error: Border width must be a positive integer"};
            }

            if (inner_border) {
                std::cout << "Inner border mode enabled (10px black border)\n";
            }

            return process_single_file(input_path, output_path, border_width, inner_border);
        }
    }

    void print_usage(const char* program_name) const override {
        std::cout << "Usage:\n";
        std::cout << "  " << program_name << " <input_image> <output_image> <border_width> [--inner]\n";
        std::cout << "  " << program_name << " <directory> <border_width> [--inner]\n\n";
        std::cout << "File mode:\n";
        std::cout << "  input_image:  Path to the input image file\n";
        std::cout << "  output_image: Path to save the output image\n";
        std::cout << "  border_width: Width of the white border in pixels\n\n";
        std::cout << "Directory mode:\n";
        std::cout << "  directory:    Path to directory containing images\n";
        std::cout << "  border_width: Width of the white border in pixels\n";
        std::cout << "                (processes all JPEG and PNG files, saves as filename_vanity_<border_width>.ext)\n\n";
        std::cout << "Options:\n";
        std::cout << "  --inner:      Add a 10px black border on the inside of the white border\n";
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
