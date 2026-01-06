#include "../command_registry.hpp"
#include "vanity/image_buffer.hpp"
#include "vanity/image_io.hpp"
#include "stb_image.h"
#include "exif.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <optional>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>
#include <cmath>

namespace vanity {

class InspectCommand : public Command {
private:
    struct ImageInfo {
        int width;
        int height;
        int channels;
    };

    struct ExifInfo {
        std::string camera;
        std::string lens;
        std::string date_time;
        int iso;
        double aperture;
        double shutter_speed;
        double focal_length;
        double gps_latitude;
        double gps_longitude;
        double gps_altitude;
        bool has_gps;
    };

    struct FileMetadata {
        std::uintmax_t file_size;
        std::filesystem::file_time_type modified_time;
        std::optional<ImageInfo> image_info;
        std::optional<ExifInfo> exif_info;
    };

    bool is_supported_image_file(const std::filesystem::path& path) const {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
    }

    std::string format_file_size(std::uintmax_t bytes) const {
        std::ostringstream oss;
        if (bytes < 1024) {
            oss << bytes << " bytes";
        } else if (bytes < 1024 * 1024) {
            oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
        } else if (bytes < 1024ULL * 1024 * 1024) {
            oss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024)) << " MB";
        } else {
            oss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024 * 1024)) << " GB";
        }
        return oss.str();
    }

    std::string format_file_size_with_bytes(std::uintmax_t bytes) const {
        std::ostringstream oss;
        oss << format_file_size(bytes) << " (" << bytes << " bytes)";
        return oss.str();
    }

    std::string format_timestamp(std::filesystem::file_time_type ftime) const {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        auto tt = std::chrono::system_clock::to_time_t(sctp);
        std::tm tm = *std::localtime(&tt);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::string format_gps_coordinate(double coord, char positive, char negative) const {
        char direction = (coord >= 0) ? positive : negative;
        double abs_coord = std::abs(coord);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4) << abs_coord << "° " << direction;
        return oss.str();
    }

    std::optional<ImageInfo> extract_image_info(const std::filesystem::path& path) const {
        int width, height, channels;
        LoadedImage img = LoadedImage::load(path.string().c_str(), width, height, channels);

        if (!img.get()) {
            return std::nullopt;
        }

        return ImageInfo{width, height, channels};
    }

    std::optional<ExifInfo> extract_exif_info(const std::filesystem::path& path) const {
        // Read entire file into memory
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return std::nullopt;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return std::nullopt;
        }

        // Parse EXIF data
        easyexif::EXIFInfo exif;
        int code = exif.parseFrom(buffer.data(), buffer.size());

        if (code != 0) {
            return std::nullopt;  // No EXIF data or parse error
        }

        // Extract relevant fields
        ExifInfo info;
        info.camera = exif.Make + (exif.Make.empty() || exif.Model.empty() ? "" : " ") + exif.Model;
        info.lens = exif.LensInfo.Model;
        info.date_time = exif.DateTime;
        info.iso = exif.ISOSpeedRatings;
        info.aperture = exif.FNumber;
        info.shutter_speed = exif.ExposureTime;
        info.focal_length = exif.FocalLength;
        info.gps_latitude = exif.GeoLocation.Latitude;
        info.gps_longitude = exif.GeoLocation.Longitude;
        info.gps_altitude = exif.GeoLocation.Altitude;

        // Check if GPS data is valid (not 0,0 and within valid ranges)
        info.has_gps = (std::abs(info.gps_latitude) > 0.0001 || std::abs(info.gps_longitude) > 0.0001) &&
                       std::abs(info.gps_latitude) <= 90.0 &&
                       std::abs(info.gps_longitude) <= 180.0;

        return info;
    }

    FileMetadata extract_metadata(const std::filesystem::path& path) const {
        FileMetadata metadata;

        // Get file size
        metadata.file_size = std::filesystem::file_size(path);

        // Get modification time
        metadata.modified_time = std::filesystem::last_write_time(path);

        // Try to extract image info
        metadata.image_info = extract_image_info(path);

        // Try to extract EXIF info (only for image files)
        if (metadata.image_info.has_value()) {
            metadata.exif_info = extract_exif_info(path);
        }

        return metadata;
    }

    void print_metadata_sectioned(const std::filesystem::path& path, const FileMetadata& metadata) const {
        std::cout << "File: " << path.string() << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

        // FILE INFORMATION section
        std::cout << "FILE INFORMATION\n";
        std::cout << "  Size:         " << format_file_size_with_bytes(metadata.file_size) << "\n";
        std::cout << "  Modified:     " << format_timestamp(metadata.modified_time) << "\n";

        // IMAGE PROPERTIES section
        if (metadata.image_info.has_value()) {
            const auto& img = metadata.image_info.value();
            std::cout << "\nIMAGE PROPERTIES\n";
            std::cout << "  Dimensions:   " << img.width << " x " << img.height << " pixels\n";
            std::cout << "  Channels:     " << img.channels;
            if (img.channels == 1) std::cout << " (Grayscale)";
            else if (img.channels == 3) std::cout << " (RGB)";
            else if (img.channels == 4) std::cout << " (RGBA)";
            std::cout << "\n";
        } else {
            std::cout << "\nNot an image file - no image metadata available.\n";
        }

        // EXIF DATA section
        if (metadata.exif_info.has_value()) {
            const auto& exif = metadata.exif_info.value();
            bool has_any_exif = !exif.camera.empty() || !exif.lens.empty() ||
                                !exif.date_time.empty() || exif.iso > 0;

            if (has_any_exif) {
                std::cout << "\nEXIF DATA\n";

                if (!exif.camera.empty()) {
                    std::cout << "  Camera:       " << exif.camera << "\n";
                }
                if (!exif.lens.empty()) {
                    std::cout << "  Lens:         " << exif.lens << "\n";
                }
                if (!exif.date_time.empty()) {
                    std::cout << "  Date Taken:   " << exif.date_time << "\n";
                }
                if (exif.iso > 0) {
                    std::cout << "  ISO:          " << exif.iso << "\n";
                }
                if (exif.aperture > 0) {
                    std::cout << "  Aperture:     f/" << std::fixed << std::setprecision(1) << exif.aperture << "\n";
                }
                if (exif.shutter_speed > 0) {
                    if (exif.shutter_speed >= 1.0) {
                        std::cout << "  Shutter:      " << std::fixed << std::setprecision(1) << exif.shutter_speed << "s\n";
                    } else {
                        std::cout << "  Shutter:      1/" << static_cast<int>(1.0 / exif.shutter_speed) << "s\n";
                    }
                }
                if (exif.focal_length > 0) {
                    std::cout << "  Focal Length: " << std::fixed << std::setprecision(0) << exif.focal_length << "mm\n";
                }
            }

            // LOCATION section
            if (exif.has_gps) {
                std::cout << "\nLOCATION\n";
                std::cout << "  GPS:          " << format_gps_coordinate(exif.gps_latitude, 'N', 'S')
                          << ", " << format_gps_coordinate(exif.gps_longitude, 'E', 'W') << "\n";
                if (exif.gps_altitude != 0) {
                    std::cout << "  Altitude:     " << std::fixed << std::setprecision(1) << exif.gps_altitude << "m\n";
                }
            }
        }

        std::cout << std::endl;
    }

    void print_metadata_simple(const std::filesystem::path& path, const FileMetadata& metadata) const {
        std::cout << "File: " << path.string() << "\n";
        std::cout << "Size: " << format_file_size_with_bytes(metadata.file_size) << "\n";
        std::cout << "Modified: " << format_timestamp(metadata.modified_time) << "\n";

        if (metadata.image_info.has_value()) {
            const auto& img = metadata.image_info.value();
            std::cout << "Width: " << img.width << " pixels\n";
            std::cout << "Height: " << img.height << " pixels\n";
            std::cout << "Channels: " << img.channels;
            if (img.channels == 1) std::cout << " (Grayscale)";
            else if (img.channels == 3) std::cout << " (RGB)";
            else if (img.channels == 4) std::cout << " (RGBA)";
            std::cout << "\n";
        } else {
            std::cout << "Not an image file\n";
        }

        if (metadata.exif_info.has_value()) {
            const auto& exif = metadata.exif_info.value();

            if (!exif.camera.empty()) {
                std::cout << "Camera: " << exif.camera << "\n";
            }
            if (!exif.lens.empty()) {
                std::cout << "Lens: " << exif.lens << "\n";
            }
            if (!exif.date_time.empty()) {
                std::cout << "Date Taken: " << exif.date_time << "\n";
            }
            if (exif.iso > 0) {
                std::cout << "ISO: " << exif.iso << "\n";
            }
            if (exif.aperture > 0) {
                std::cout << "Aperture: f/" << std::fixed << std::setprecision(1) << exif.aperture << "\n";
            }
            if (exif.shutter_speed > 0) {
                std::cout << "Shutter: ";
                if (exif.shutter_speed >= 1.0) {
                    std::cout << std::fixed << std::setprecision(1) << exif.shutter_speed << "s\n";
                } else {
                    std::cout << "1/" << static_cast<int>(1.0 / exif.shutter_speed) << "s\n";
                }
            }
            if (exif.focal_length > 0) {
                std::cout << "Focal Length: " << std::fixed << std::setprecision(0) << exif.focal_length << "mm\n";
            }
            if (exif.has_gps) {
                std::cout << "GPS: " << format_gps_coordinate(exif.gps_latitude, 'N', 'S')
                          << ", " << format_gps_coordinate(exif.gps_longitude, 'E', 'W') << "\n";
                if (exif.gps_altitude != 0) {
                    std::cout << "Altitude: " << std::fixed << std::setprecision(1) << exif.gps_altitude << "m\n";
                }
            }
        }

        std::cout << std::endl;
    }

    CommandResult process_single_file(const std::filesystem::path& path, const std::string& format) const {
        namespace fs = std::filesystem;

        // Check if file exists
        if (!fs::exists(path)) {
            return {1, "Error: File not found: " + path.string()};
        }

        // Extract metadata
        FileMetadata metadata = extract_metadata(path);

        // Print metadata based on format
        if (format == "simple") {
            print_metadata_simple(path, metadata);
        } else {
            print_metadata_sectioned(path, metadata);
        }

        return {0, ""};
    }

public:
    CommandResult execute(int argc, char* argv[]) override {
        namespace fs = std::filesystem;

        // Parse format flag and collect file arguments
        std::string format = "sectioned";  // default
        std::vector<std::string> files;

        for (int i = 1; i < argc; i++) {
            std::string arg(argv[i]);
            if (arg.find("--format=") == 0) {
                format = arg.substr(9);
                if (format != "sectioned" && format != "simple") {
                    return {1, "Error: Invalid format. Use 'sectioned' or 'simple'"};
                }
            } else {
                files.push_back(arg);
            }
        }

        // Check if we have any files
        if (files.empty()) {
            print_usage(argv[0]);
            return {1, ""};
        }

        // Check if single argument is a directory
        if (files.size() == 1 && fs::exists(files[0]) && fs::is_directory(files[0])) {
            // Directory mode
            fs::path directory(files[0]);

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

            // Process each image file
            for (size_t i = 0; i < image_files.size(); i++) {
                CommandResult result = process_single_file(image_files[i], format);
                if (result.exit_code != 0) {
                    std::cerr << result.message << "\n";
                }

                // Add separator between files (but not after the last one)
                if (i < image_files.size() - 1) {
                    if (format == "sectioned") {
                        std::cout << "\n";
                    }
                }
            }

            return {0, ""};
        } else {
            // Single or multiple file mode
            for (size_t i = 0; i < files.size(); i++) {
                CommandResult result = process_single_file(fs::path(files[i]), format);
                if (result.exit_code != 0) {
                    std::cerr << result.message << "\n";
                }

                // Add separator between files (but not after the last one)
                if (i < files.size() - 1) {
                    if (format == "sectioned") {
                        std::cout << "\n";
                    }
                }
            }

            return {0, ""};
        }
    }

    void print_usage(const char* program_name) const override {
        std::cout << "Usage:\n";
        std::cout << "  " << program_name << " [--format=FORMAT] <file>\n";
        std::cout << "  " << program_name << " [--format=FORMAT] <file1> <file2> ...\n";
        std::cout << "  " << program_name << " [--format=FORMAT] <directory>\n\n";
        std::cout << "Display file metadata including dimensions, EXIF data, and filesystem info.\n\n";
        std::cout << "Arguments:\n";
        std::cout << "  file:         Path to a file to inspect\n";
        std::cout << "  directory:    Path to directory (processes all JPEG and PNG files)\n\n";
        std::cout << "Options:\n";
        std::cout << "  --format=FORMAT  Output format: 'sectioned' (default) or 'simple'\n\n";
        std::cout << "Metadata displayed:\n";
        std::cout << "  - File size and modification time\n";
        std::cout << "  - Image dimensions (width, height, channels)\n";
        std::cout << "  - EXIF data (camera, lens, settings, date taken)\n";
        std::cout << "  - GPS location and altitude (if available)\n";
    }

    const char* name() const override {
        return "inspect";
    }

    const char* description() const override {
        return "Display file metadata including image dimensions and EXIF data";
    }
};

// Factory function to create the command
std::unique_ptr<Command> create_inspect_command() {
    return std::make_unique<InspectCommand>();
}

} // namespace vanity
