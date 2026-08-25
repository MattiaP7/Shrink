/**
 * @file Config.hpp
 * @author Mattia Pirazzi (7mattiapirazzi@gmail.com)
 * @brief Command line configuration parsing and options for shrink.
 * @version 0.1
 * @date 2026-08-25
 *
 * @copyright Copyright (c) 2026 shrink Project. Licensed under the MIT License.
 *
 */
#pragma once

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include <cxxopts.hpp>

namespace fs = std::filesystem;

/**
 * @brief Command-line configuration for a compression run.
 */
struct Config {
  fs::path target_path;                ///< Input file or directory path
  std::optional<fs::path> output_path; ///< Custom output destination path

  bool is_single_file{false}; ///< True if target is a file, false if directory
  bool recursive{false};      ///< Enable recursive sub-directory processing

  float quality{80.0f};         ///< WebP compression quality (0.0 to 100.0)
  std::size_t threads_count{0}; ///< Number of worker threads (0 = auto-detect)

  std::optional<int>
      max_width; ///< Max output width while preserving aspect ratio
  std::optional<int>
      max_height; ///< Max output height while preserving aspect ratio

  std::optional<fs::path>
      benchmark_export_path; ///< Path for exporting JSON benchmark metrics

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

    options.add_options()("f,file", "Path to a single image file",
                          cxxopts::value<std::string>())(
        "d,directory", "Path to a directory containing images",
        cxxopts::value<std::string>())(
        "o,output", "Output directory or file path (optional)",
        cxxopts::value<std::string>())(
        "r,recursive", "Recursively process images in subdirectories",
        cxxopts::value<bool>()->default_value("false"))(
        "q,quality", "Compression quality (0.0 - 100.0)",
        cxxopts::value<float>()->default_value("80.0"))(
        "t,threads", "Worker threads count (0 for hardware auto-detect)",
        cxxopts::value<std::size_t>()->default_value("0"))(
        "max-width", "Maximum image width in pixels", cxxopts::value<int>())(
        "max-height", "Maximum image height in pixels", cxxopts::value<int>())(
        "benchmark", "Export detailed compression metrics to JSON file",
        cxxopts::value<std::string>())("h,help", "Print CLI usage and options");

    try {
      auto result = options.parse(argc, argv);

      if (result.count("help")) {
        std::cout << options.help() << "\n";
        return std::nullopt;
      }

      const bool has_file = result.count("file") > 0;
      const bool has_dir = result.count("directory") > 0;

      if (has_file && has_dir) {
        std::cerr << "Error: Cannot specify both -f/--file and -d/--directory "
                     "simultaneously.\n\n";
        std::cout << options.help() << "\n";
        return std::nullopt;
      }

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

      if (result.count("output")) {
        cfg.output_path = result["output"].as<std::string>();
      }

      cfg.recursive = result["recursive"].as<bool>();
      cfg.quality = result["quality"].as<float>();

      // Validazione del range di qualità
      if (cfg.quality < 0.0f || cfg.quality > 100.0f) {
        std::cerr << "Error: Quality must be between 0.0 and 100.0.\n";
        return std::nullopt;
      }

      cfg.threads_count = result["threads"].as<std::size_t>();
      if (cfg.threads_count == 0) {
        const auto hardware_threads = std::thread::hardware_concurrency();
        cfg.threads_count = (hardware_threads > 0) ? hardware_threads : 4;
      }

      if (result.count("max-width")) {
        cfg.max_width = result["max-width"].as<int>();
      }
      if (result.count("max-height")) {
        cfg.max_height = result["max-height"].as<int>();
      }
      if (result.count("benchmark")) {
        cfg.benchmark_export_path = result["benchmark"].as<std::string>();
      }

      return cfg;
    } catch (const cxxopts::exceptions::exception &e) {
      std::cerr << "Option parsing error: " << e.what() << "\n\n";
      std::cout << options.help() << "\n";
      return std::nullopt;
    }
  }
};
