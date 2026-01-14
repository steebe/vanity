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
    // Instagram aspect ratio limits
    static constexpr float RATIO_LANDSCAPE = 1.91f; // 1.91:1 (wide)
    static constexpr float RATIO_PORTRAIT = 0.75f;  // 3:4 (tall)
    static constexpr int MIN_WIDTH = 320;
    static constexpr int MAX_WIDTH = 1080;

    bool is_supported_image_file(const std::filesystem::path &path) const
    {
      std::string ext = path.extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
    }

    // Calculate target dimensions after padding to fit Instagram ratio
    void calculate_padded_dimensions(int src_width, int src_height,
                                     int &dst_width, int &dst_height) const
    {
      float ratio = static_cast<float>(src_width) / src_height;

      if (ratio > RATIO_LANDSCAPE)
      {
        // Too wide - add vertical padding to reach 1.91:1
        dst_width = src_width;
        dst_height = static_cast<int>(std::ceil(src_width / RATIO_LANDSCAPE));
      }
      else if (ratio < RATIO_PORTRAIT)
      {
        // Too tall - add horizontal padding to reach 3:4 (0.75)
        dst_height = src_height;
        dst_width = static_cast<int>(std::ceil(src_height * RATIO_PORTRAIT));
      }
      else
      {
        // Within Instagram's supported range - no padding needed
        dst_width = src_width;
        dst_height = src_height;
      }
    }

    // Calculate final dimensions after scaling to fit Instagram width requirements
    // total_border: combined border thickness (inner + outer) to reserve space for border
    void calculate_scaled_dimensions(int src_width, int src_height,
                                     int &dst_width, int &dst_height,
                                     int total_border = 0) const
    {
      // Account for border space in the target dimensions
      int effective_max = MAX_WIDTH - 2 * total_border;
      int effective_min = MIN_WIDTH - 2 * total_border;

      // Ensure minimums stay reasonable
      if (effective_max < 1)
        effective_max = 1;
      if (effective_min < 1)
        effective_min = 1;

      if (src_width > effective_max)
      {
        // Scale down to effective max width (leaving room for border)
        float scale = static_cast<float>(effective_max) / src_width;
        dst_width = effective_max;
        dst_height = static_cast<int>(std::round(src_height * scale));
      }
      else if (src_width < effective_min)
      {
        // Scale up to effective min width
        float scale = static_cast<float>(effective_min) / src_width;
        dst_width = effective_min;
        dst_height = static_cast<int>(std::round(src_height * scale));
      }
      else
      {
        // Width is within range - keep as is
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

      float original_ratio = static_cast<float>(width) / height;
      std::cout << "Original aspect ratio: " << original_ratio << "\n";

      // Step 1: Calculate if padding is needed
      int padded_width, padded_height;
      calculate_padded_dimensions(width, height, padded_width, padded_height);

      unsigned char *current_data = img.get();
      int current_width = width;
      int current_height = height;
      std::optional<ImageBuffer> padded_buffer;

      if (padded_width != width || padded_height != height)
      {
        // Need to add padding
        padded_buffer.emplace(padded_width, padded_height, channels);
        unsigned char black[4] = {0, 0, 0, 255};

        if (!add_padding(img.get(), width, height, channels,
                         padded_buffer->get(), padded_width, padded_height, black))
        {
          return {1, "Error: Failed to add padding"};
        }

        current_data = padded_buffer->get();
        current_width = padded_width;
        current_height = padded_height;

        float target_ratio = (original_ratio > RATIO_LANDSCAPE) ? RATIO_LANDSCAPE : RATIO_PORTRAIT;
        std::cout << "Added padding to reach aspect ratio " << target_ratio
                  << " (" << padded_width << "x" << padded_height << ")\n";
      }
      else
      {
        std::cout << "Aspect ratio within Instagram range, no padding needed\n";
      }

      // Step 2: Scale to fit Instagram width requirements (reserving space for border)
      // Inner border is asymmetric: 5px horizontal (left/right), 10px vertical (top/bottom)
      const int inner_border_h = inner_border ? 5 : 0;
      const int inner_border_v = inner_border ? 10 : 0;
      int total_border_h = border_width + inner_border_h;
      int total_border_v = border_width + inner_border_v;

      int final_width, final_height;
      calculate_scaled_dimensions(current_width, current_height, final_width, final_height, total_border_h);

      std::optional<ImageBuffer> scaled_buffer;

      if (final_width != current_width || final_height != current_height)
      {
        // Need to scale
        scaled_buffer.emplace(final_width, final_height, channels);

        if (!resize_image(current_data, current_width, current_height, channels,
                          scaled_buffer->get(), final_width, final_height))
        {
          return {1, "Error: Failed to resize image"};
        }

        current_data = scaled_buffer->get();
        current_width = final_width;
        current_height = final_height;

        std::cout << "Scaled to " << final_width << "x" << final_height;
        if (total_border_h > 0 || total_border_v > 0)
        {
          std::cout << " (reserving " << total_border_h << "px H, " << total_border_v << "px V for border)";
        }
        std::cout << "\n";
      }
      else
      {
        std::cout << "Width within Instagram range, no scaling needed\n";
      }

      // Step 3: Add border if requested (after scaling to preserve original resolution)
      std::optional<ImageBuffer> inner_border_buffer;
      std::optional<ImageBuffer> border_buffer;

      if (border_width > 0)
      {
        // Add inner black border if requested (asymmetric: 5px H, 10px V)
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
          std::cout << "Added " << inner_border_h << "px H, " << inner_border_v << "px V black inner border\n";
        }

        // Add white or gradient border
        int bordered_width, bordered_height;
        calculate_bordered_dimensions(current_width, current_height, border_width,
                                      bordered_width, bordered_height);

        border_buffer.emplace(bordered_width, bordered_height, channels);

        if (gradient)
        {
          // Calculate average color of the current image
          unsigned char avg_color[4];
          calculate_average_color(current_data, current_width, current_height, channels, avg_color);

          std::cout << "Average color: RGB(" << static_cast<int>(avg_color[0]) << ", "
                    << static_cast<int>(avg_color[1]) << ", "
                    << static_cast<int>(avg_color[2]) << ")\n";

          if (!add_gradient_border(current_data, current_width, current_height, channels,
                                   border_buffer->get(), border_width, avg_color))
          {
            return {1, "Error: Failed to add gradient border"};
          }

          current_data = border_buffer->get();
          current_width = bordered_width;
          current_height = bordered_height;
          std::cout << "Added " << border_width << "px gradient border (average color to white)\n";
        }
        else
        {
          unsigned char white[4] = {255, 255, 255, 255};

          if (!add_border(current_data, current_width, current_height, channels,
                          border_buffer->get(), border_width, white))
          {
            return {1, "Error: Failed to add border"};
          }

          current_data = border_buffer->get();
          current_width = bordered_width;
          current_height = bordered_height;
          std::cout << "Added " << border_width << "px white border\n";
        }
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
      std::cout << "Prepares images for Instagram by adjusting aspect ratio and dimensions.\n\n";
      std::cout << "Instagram requirements:\n";
      std::cout << "  - Width: 320-1080 pixels\n";
      std::cout << "  - Aspect ratio: between 1.91:1 (landscape) and 3:4 (portrait)\n\n";
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
      std::cout << "  - If aspect ratio is outside Instagram's range, black padding is added\n";
      std::cout << "  - Images are scaled to fit within 320-1080px width (accounting for border)\n";
      std::cout << "  - Border is applied after scaling (preserving original image resolution)\n";
      std::cout << "  - No cropping is performed\n";
    }

    const char *name() const override
    {
      return "instafy";
    }

    const char *description() const override
    {
      return "Prepare images for Instagram (adjust aspect ratio and dimensions)";
    }
  };

  // Factory function to create the command (called from commands.cpp)
  std::unique_ptr<Command> create_instafy_command()
  {
    return std::make_unique<InstafyCommand>();
  }

} // namespace vanity
