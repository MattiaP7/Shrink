/**
 * @file Application.hpp
 * @author Mattia Pirazzi
 * @brief Declares the Application class, which orchestrates image discovery,
 * compression, and reporting.
 * @version 0.1
 * @date 2026-08-24
 *
 * @copyright Copyright (c) 2026 Shrink Project. Licensed under the MIT License.
 *
 */
#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

#include "Config.hpp"
#include "ImageProcessor.hpp"

namespace fs = std::filesystem;

/**
 * @brief Main application class that manages image discovery, compression, and
 * reporting.
 *
 */
class Application {
public:
  /**
   * @brief Creates an application from the parsed command-line configuration.
   * @param config Runtime options controlling the input and compression.
   */
  explicit Application(Config config);

  /**
   * @brief Processes all images discovered from the configured input path.
   * @return Process exit status. A value of zero indicates normal completion.
   */
  int run();

private:
  Config cfg_;
  std::vector<fs::path> image_to_process_;

  /** @brief Populates image_to_process_ from the configured file or directory.
   */
  void collect_images();

  /**
   * @brief Checks whether a path has a supported raster-image extension.
   * @param path Path whose extension should be checked.
   * @return True for PNG, JPG, JPEG, or BMP files, case-insensitively.
   */
  static bool is_supported_image(const fs::path &path);

  /**
   * @brief Logs aggregate compression statistics.
   * @param success_count Number of successfully processed images.
   * @param total_count Number of images discovered for processing.
   * @param orig_bytes Combined size of the source files.
   * @param comp_bytes Combined size of the generated WebP files.
   */
  static void print_summary(size_t success_count, size_t total_count,
                            uint64_t orig_bytes, uint64_t comp_bytes);

  /**
   * @brief Writes per-image compression metrics as a JSON benchmark report.
   * @param export_path Destination path for the report.
   * @param results Successful compression results to serialize.
   * @param total_duration_ms Total wall-clock processing time in milliseconds.
   * @param threads_count Number of worker threads used.
   */
  static void
  export_benchmark_json(const fs::path &export_path,
                        const std::vector<CompressionResult> &results,
                        double total_duration_ms, size_t threads_count);
};

#endif
