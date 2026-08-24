#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include <cxxopts.hpp>

/**
 * @brief Command-line configuration for a compression run.
 */
struct Config {
  /** @brief Input file or directory selected by the user. */
  std::filesystem::path target_path;
  /** @brief True when target_path identifies one file rather than a directory.
   */
  bool is_single_file = false;
  /** @brief WebP quality in the range 0.0 to 100.0. */
  float quality = 80.0f;
  /** @brief Number of workers; zero requests automatic detection. */
  std::size_t threads_count = 0;

  /** @brief Optional maximum output width while preserving the aspect ratio. */
  std::optional<int> max_width;
  /** @brief Optional maximum output height while preserving the aspect ratio.
   */
  std::optional<int> max_height;

  /** @brief Optional destination for the detailed benchmark JSON report. */
  std::optional<std::filesystem::path> benchmark_export_path;

  /**
   * @brief Parses command-line arguments into a validated configuration.
   * @param argc Argument count received by main().
   * @param argv Argument values received by main().
   * @return A configuration, or std::nullopt after help or invalid input.
   */
  static std::optional<Config> parse(int argc, char *argv[]) {
    cxxopts::Options options(
        "shrink",
        "Ultra-efficient C++23 CLI for batch image compression to WebP");

    options.add_options()("f,file", "Path of a single image file",
                          cxxopts::value<std::string>())(
        "d,directory", "Directory containing images",
        cxxopts::value<std::string>())(
        "q,quality", "Compression quality (0.0 - 100.0)",
        cxxopts::value<float>()->default_value("80.0"))(
        "t,threads", "Number of threads (0 for auto-detect)",
        cxxopts::value<std::size_t>()->default_value("0"))(
        "max-width", "Maximum image width (preserves aspect ratio)",
        cxxopts::value<int>())(
        "max-height", "Maximum image height (preserves aspect ratio)",
        cxxopts::value<int>())("h,help", "Print help message")(
        "benchmark", "Export detailed compression metrics to JSON",
        cxxopts::value<std::string>());

    try {
      auto result = options.parse(argc, argv);

      if (result.count("help")) {
        std::cout << options.help() << "\n";
        return std::nullopt;
      }

      const bool has_file = result.count("file") > 0;
      const bool has_dir = result.count("directory") > 0;

      if (!has_file && !has_dir) {
        std::cerr << "Error: You must specify either -f/--file or "
                     "-d/--directory.\n\n";
        std::cout << options.help() << "\n";
        return std::nullopt;
      }

      Config cfg;
      if (has_file) {
        cfg.target_path = result["file"].as<std::string>();
        cfg.is_single_file = true;
      } else {
        cfg.target_path = result["directory"].as<std::string>();
        cfg.is_single_file = false;
      }

      cfg.quality = result["quality"].as<float>();
      cfg.threads_count = result["threads"].as<std::size_t>();

      if (cfg.threads_count == 0) {
        cfg.threads_count = std::thread::hardware_concurrency();
      }

      if (result.count("max-width")) {
        cfg.max_width = result["max-width"].as<int>();
      }
      if (result.count("max-height")) {
        cfg.max_height = result["max-height"].as<int>();
      }

      if (result.count("benchmark"))
        cfg.benchmark_export_path = result["benchmark"].as<std::string>();

      return cfg;
    } catch (const cxxopts::exceptions::exception &e) {
      std::cerr << "Option parsing error: " << e.what() << "\n";
      return std::nullopt;
    }
  }
};

#endif
