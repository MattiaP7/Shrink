#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include <cxxopts.hpp>

struct Config {
  std::filesystem::path target_path;
  bool is_single_file = false;
  float quality = 80.0f;
  std::size_t threads_count = 0;

  // Parametri per il ridimensionamento
  std::optional<int> max_width;
  std::optional<int> max_height;

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
        cxxopts::value<int>())("h,help", "Print help message");

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

      return cfg;
    } catch (const cxxopts::exceptions::exception &e) {
      std::cerr << "Option parsing error: " << e.what() << "\n";
      return std::nullopt;
    }
  }
};

#endif