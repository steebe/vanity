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
#include <cmath>
#include <optional>

namespace vanity
{
  class InstafyCommand : public Command
  {
  private:
    // Target dimensions for all output images
    static constexpr int TARGET_SIZE = 1080;

    bool is_supported_image_file(const std::filesystem::path &path) const
    {
      std::string ext = path.extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
    }

    // Calculate dimensions to fit image into TARGET_SIZE x TARGET_SIZE while preserving aspect ratio
    void calculate_fit_dimensions(int src_width, int src_height,
                                  int &dst_width, int &dst_height) const
    {
      float ratio = static_cast<float>(src_width) / src_height;

      if (src_width > TARGET_SIZE || src_height > TARGET_SIZE)
      {
        // Image is larger than target - scale down to fit
        if (src_width >= src_height)
        {
          // Landscape or square - width is limiting dimension
          dst_width = TARGET_SIZE;
          dst_height = static_cast<int>(std::round(TARGET_SIZE / ratio));
        }
        else
        {
          // Portrait - height is limiting dimension
          dst_height = TARGET_SIZE;
          dst_width = static_cast<int>(std::round(TARGET_SIZE * ratio));
        }
      }
      else
      {
        // Image is smaller than target - keep original size
        dst_width = src_width;
        dst_height = src_height;
      }
    }

    CommandResult process_single_file(const char *input_path, const char *output_path,
                                      int border_width, bool inner_border, bool gradient)
    {
      // Load image
      int width, height, channels;
      LoadedImage img = LoadedImage::load(input_path, width, height, channels);

      if (!img.get())
      {
        std::string error = "Error: Failed to load image '";
        error += input_path;
        error += "'\nReason: ";
        error += stbi_failure_reason();
        return {1, error};
      }

      std::cout << "Loaded image: " << width << "x" << height
                << " with " << channels << " channels\n";

      unsigned char *current_data = img.get();
      int current_width = width;
      int current_height = height;

      // Calculate border dimensions
      const int inner_border_h = inner_border ? 5 : 0;
      const int inner_border_v = inner_border ? 10 : 0;

      // Step 1: Add inner border if requested (wraps actual image content)
      std::optional<ImageBuffer> inner_border_buffer;

      if (inner_border)
      {
        int inner_width, inner_height;
        calculate_bordered_dimensions(current_width, current_height,
                                      inner_border_h, inner_border_v,
                                      inner_width, inner_height);

        inner_border_buffer.emplace(inner_width, inner_height, channels);
        unsigned char black[4] = {0, 0, 0, 255};

        if (!add_border(current_data, current_width, current_height, channels,
                        inner_border_buffer->get(), inner_border_h, inner_border_v, black))
        {
          return {1, "Error: Failed to add inner border"};
        }

        current_data = inner_border_buffer->get();
        current_width = inner_width;
        current_height = inner_height;
        std::cout << "Added " << inner_border_h << "px H, " << inner_border_v << "px V black inner border (wrapping original content)\n";
      }

      // Calculate the content area size (accounting for outer border only)
      const int total_border_h = (border_width * 2);
      const int total_border_v = (border_width * 2);
      const int content_width = TARGET_SIZE - total_border_h;
      const int content_height = TARGET_SIZE - total_border_v;

      std::cout << "Content area will be: " << content_width << "x" << content_height << "\n";

      // Step 2: Scale image to fit within the content area
      int fitted_width, fitted_height;
      {
        float ratio = static_cast<float>(current_width) / current_height;

        if (current_width > content_width || current_height > content_height)
        {
          // Image is larger than content area - scale down to fit
          if (current_width >= current_height)
          {
            // Landscape or square - width is limiting dimension
            fitted_width = content_width;
            fitted_height = static_cast<int>(std::round(content_width / ratio));

            // Ensure height also fits
            if (fitted_height > content_height)
            {
              fitted_height = content_height;
              fitted_width = static_cast<int>(std::round(content_height * ratio));
            }
          }
          else
          {
            // Portrait - height is limiting dimension
            fitted_height = content_height;
            fitted_width = static_cast<int>(std::round(content_height * ratio));

            // Ensure width also fits
            if (fitted_width > content_width)
            {
              fitted_width = content_width;
              fitted_height = static_cast<int>(std::round(content_width / ratio));
            }
          }
        }
        else
        {
          // Image is smaller than content area - keep original size
          fitted_width = current_width;
          fitted_height = current_height;
        }
      }

      std::optional<ImageBuffer> scaled_buffer;

      if (fitted_width != current_width || fitted_height != current_height)
      {
        scaled_buffer.emplace(fitted_width, fitted_height, channels);

        if (!resize_image(current_data, current_width, current_height, channels,
                          scaled_buffer->get(), fitted_width, fitted_height))
        {
          return {1, "Error: Failed to resize image"};
        }

        current_data = scaled_buffer->get();
        current_width = fitted_width;
        current_height = fitted_height;

        std::cout << "Scaled to " << fitted_width << "x" << fitted_height << " to fit within content area\n";
      }

      // Step 3: Add padding to center within content area
      std::optional<ImageBuffer> padded_buffer;
      unsigned char padding_color[4] = {255, 255, 255, 255}; // White padding

      if (current_width != content_width || current_height != content_height)
      {
        padded_buffer.emplace(content_width, content_height, channels);

        if (!add_padding(current_data, current_width, current_height, channels,
                         padded_buffer->get(), content_width, content_height, padding_color))
        {
          return {1, "Error: Failed to add padding"};
        }

        current_data = padded_buffer->get();
        current_width = content_width;
        current_height = content_height;

        std::cout << "Added padding to center within " << content_width << "x" << content_height << " content area\n";
      }

      // Step 4: Add outer border (white or gradient) if requested
      std::optional<ImageBuffer> border_buffer;

      if (border_width > 0)
      {
        int bordered_width, bordered_height;
        calculate_bordered_dimensions(current_width, current_height, border_width,
                                      bordered_width, bordered_height);

        border_buffer.emplace(bordered_width, bordered_height, channels);

        if (gradient)
        {
          // For gradient, we want to use the original image data (before any borders/padding)
          // Always use the original loaded image for color calculation
          unsigned char avg_color[4];
          calculate_average_color(img.get(), width, height, channels, avg_color);

          std::cout << "Average color: RGB(" << static_cast<int>(avg_color[0]) << ", "
                    << static_cast<int>(avg_color[1]) << ", "
                    << static_cast<int>(avg_color[2]) << ")\n";

          if (!add_gradient_border(current_data, current_width, current_height, channels,
                                   border_buffer->get(), border_width, avg_color))
          {
            return {1, "Error: Failed to add gradient border"};
          }

          std::cout << "Added " << border_width << "px gradient border (average color to white)\n";
        }
        else
        {
          unsigned char border_color[4] = {255, 255, 255, 255}; // White border

          if (!add_border(current_data, current_width, current_height, channels,
                          border_buffer->get(), border_width, border_color))
          {
            return {1, "Error: Failed to add border"};
          }

          std::cout << "Added " << border_width << "px white border\n";
        }

        current_data = border_buffer->get();
        current_width = bordered_width;
        current_height = bordered_height;
      }

      // Verify we reached the target size
      if (current_width != TARGET_SIZE || current_height != TARGET_SIZE)
      {
        return {1, "Error: Final image size mismatch - expected " +
                 std::to_string(TARGET_SIZE) + "x" + std::to_string(TARGET_SIZE) +
                 " but got " + std::to_string(current_width) + "x" + std::to_string(current_height)};
      }

      // Write output image
      if (!write_image(output_path, current_width, current_height, channels, current_data))
      {
        return {1, "Error: Failed to write image"};
      }

      std::cout << "Successfully wrote image: " << current_width << "x" << current_height
                << " to '" << output_path << "'\n";

      return {0, ""};
    }

  public:
    CommandResult execute(int argc, char *argv[]) override
    {
      namespace fs = std::filesystem;

      // Parse flags and collect positional arguments
      int border_width = 0;
      bool inner_border = false;
      bool gradient = false;
      std::vector<std::string> args;

      for (int i = 1; i < argc; i++)
      {
        std::string arg = argv[i];
        if (arg == "--border" && i + 1 < argc)
        {
          border_width = std::atoi(argv[++i]);
          if (border_width <= 0)
          {
            return {1, "Error: Border width must be a positive integer"};
          }
        }
        else if (arg == "--inner")
        {
          inner_border = true;
        }
        else if (arg == "--gradient")
        {
          gradient = true;
        }
        else
        {
          args.push_back(arg);
        }
      }

      if (inner_border && border_width == 0)
      {
        return {1, "Error: --inner requires --border <width>"};
      }

      if (gradient && border_width == 0)
      {
        return {1, "Error: --gradient requires --border <width>"};
      }

      // Support two modes:
      // 1. File mode: vanity instafy <input_image> <output_image>
      // 2. Directory mode: vanity instafy <directory>
      if (args.size() != 1 && args.size() != 2)
      {
        print_usage(argv[0]);
        return {1, ""};
      }

      if (args.size() == 1)
      {
        // Could be directory mode or invalid
        fs::path path(args[0]);

        if (fs::is_directory(path))
        {
          // Directory mode
          std::vector<fs::path> image_files;
          for (const auto &entry : fs::directory_iterator(path))
          {
            if (entry.is_regular_file() && is_supported_image_file(entry.path()))
            {
              image_files.push_back(entry.path());
            }
          }

          if (image_files.empty())
          {
            return {1, "Error: No JPEG or PNG files found in directory"};
          }

          std::cout << "Found " << image_files.size() << " image file(s) to process\n";

          int success_count = 0;
          int failure_count = 0;

          for (const auto &input_file : image_files)
          {
            std::string stem = input_file.stem().string();
            std::string extension = input_file.extension().string();
            std::string output_filename = stem + "_insta" + extension;
            fs::path output_file = input_file.parent_path() / output_filename;

            std::cout << "\nProcessing: " << input_file.filename().string()
                      << " -> " << output_filename << "\n";

            CommandResult result = process_single_file(
                input_file.string().c_str(),
                output_file.string().c_str(),
                border_width,
                inner_border,
                gradient);

            if (result.exit_code == 0)
            {
              success_count++;
            }
            else
            {
              failure_count++;
              std::cerr << result.message << "\n";
            }
          }

          std::cout << "\nCompleted: " << success_count << " successful, "
                    << failure_count << " failed\n";
          return {failure_count > 0 ? 1 : 0, ""};
        }
        else
        {
          // Single file without output path - error
          print_usage(argv[0]);
          return {1, "Error: For single file mode, please specify both input and output paths"};
        }
      }
      else
      {
        // File mode (2 arguments)
        const char *input_path = args[0].c_str();
        const char *output_path = args[1].c_str();

        return process_single_file(input_path, output_path, border_width, inner_border, gradient);
      }
    }

    void print_usage(const char *program_name) const override
    {
      std::cout << "Usage:\n";
      std::cout << "  " << program_name << " <input_image> <output_image> [options]\n";
      std::cout << "  " << program_name << " <directory> [options]\n\n";
      std::cout << "Converts images to consistent 1080x1080 format for Instagram.\n\n";
      std::cout << "File mode:\n";
      std::cout << "  input_image:  Path to the input image file\n";
      std::cout << "  output_image: Path to save the output image\n\n";
      std::cout << "Directory mode:\n";
      std::cout << "  directory:    Path to directory containing images\n";
      std::cout << "                (processes all JPEG and PNG files, saves as filename_insta.ext)\n\n";
      std::cout << "Options:\n";
      std::cout << "  --border <width>  Add a white border of specified width (in pixels)\n";
      std::cout << "  --gradient        Apply a gradient border from average image color to white\n";
      std::cout << "                    (requires --border)\n";
      std::cout << "  --inner           Add a black border inside the white border\n";
      std::cout << "                    (5px left/right, 10px top/bottom; requires --border)\n\n";
      std::cout << "Behavior:\n";
      std::cout << "  - All output images are exactly 1080x1080 pixels\n";
      std::cout << "  - Original image is scaled to fit within the content area\n";
      std::cout << "  - White padding is added within the content area to center the image\n";
      std::cout << "  - Borders are added around the padded content\n";
      std::cout << "  - No cropping is performed\n";
    }

    const char *name() const override
    {
      return "instafy";
    }

    const char *description() const override
    {
      return "Convert images to consistent 1080x1080 format for Instagram";
    }
  };

  // Factory function to create the command (called from commands.cpp)
  std::unique_ptr<Command> create_instafy_command()
  {
    return std::make_unique<InstafyCommand>();
  }

} // namespace vanity
