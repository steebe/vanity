#include "../command_registry.hpp"
#include "vanity/image_buffer.hpp"
#include "vanity/image_ops.hpp"
#include "vanity/image_io.hpp"
#include "stb_image.h"
#include <iostream>
#include <cstdlib>

namespace vanity {

class AddBorderCommand : public Command {
public:
    CommandResult execute(int argc, char* argv[]) override {
        if (argc != 4) {
            print_usage(argv[0]);
            return {1, ""};
        }

        const char* input_path = argv[1];
        const char* output_path = argv[2];
        int border_width = std::atoi(argv[3]);

        if (border_width <= 0) {
            return {1, "Error: Border width must be a positive integer"};
        }

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

    void print_usage(const char* program_name) const override {
        std::cout << "Usage: " << program_name << " <input_image> <output_image> <border_width>\n";
        std::cout << "  input_image:  Path to the input image file\n";
        std::cout << "  output_image: Path to save the output image\n";
        std::cout << "  border_width: Width of the white border in pixels\n";
    }

    const char* name() const override {
        return "add_border";
    }

    const char* description() const override {
        return "Add white border around an image";
    }
};

// Factory function to create the command (called from commands.cpp)
std::unique_ptr<Command> create_add_border_command() {
    return std::make_unique<AddBorderCommand>();
}

} // namespace vanity
