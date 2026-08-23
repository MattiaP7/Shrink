#include <filesystem>
#include <iostream>
#include <ranges>
#include <vector>

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

#include "../include/ImageProcessor.hpp"
#include "../include/ThreadPool.hpp"

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {

  cxxopts::Options options("shrink", "Ultra-efficient C++23 CLI for batch "
                                     "image compression to WebP");

  options.add_options()("d,directory", "Directory containing the images",
                        cxxopts::value<std::string>())(
      "q,quality", "Compression quality (0.0 - 100.0)",
      cxxopts::value<float>()->default_value("80.0"))(
      "t,threads", "Number of threads (0 for auto-detect)",
      cxxopts::value<size_t>()->default_value("0"))("h,help",
                                                    "Print help message");

  auto result = options.parse(argc, argv);

  if (result.count("help") || !result.count("directory")) {
    std::cout << options.help() << "\n";
    return 0;
  }

  fs::path target_dir = result["directory"].as<std::string>();
  float quality = result["quality"].as<float>();
  size_t threads_count = result["threads"].as<size_t>();

  if (threads_count == 0) {
    threads_count = std::thread::hardware_concurrency();
  }

  if (!fs::is_directory(target_dir)) {
    spdlog::error("Error: The path '{}' is not a valid directory.",
                  target_dir.string());
    return 1;
  }

  // Filter supported extensions
  auto is_supported_image = [](const fs::directory_entry &entry) {
    if (!entry.is_regular_file())
      return false;
    auto ext = entry.path().extension().string();
    for (auto &c : ext)
      c = static_cast<char>(std::tolower(c));
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
  };

  std::vector<fs::path> images_to_process;
  for (const auto &entry : fs::directory_iterator(target_dir) |
                               std::views::filter(is_supported_image)) {
    images_to_process.push_back(entry.path());
  }

  if (images_to_process.empty()) {
    spdlog::warn("No supported images (.png, .jpg, .jpeg, .bmp) found in the "
                 "directory.");
    return 0;
  }

  spdlog::info(
      "Found {} images. Starting processing with {} threads (Quality: {})...",
      images_to_process.size(), threads_count, quality);

  ThreadPool pool(threads_count);
  std::vector<std::future<std::expected<CompressionResult, CompressionError>>>
      futures;

  for (const auto &img_path : images_to_process) {
    futures.push_back(pool.enqueue([img_path, quality]() {
      return ImageProcessor::compress_to_webp(img_path, quality);
    }));
  }

  uint64_t total_orig_bytes = 0;
  uint64_t total_comp_bytes = 0;
  size_t success_count = 0;

  constexpr double bytes_per_mb = 1024.0 * 1024.0;

  for (auto &fut : futures) {
    auto res = fut.get();
    if (res.has_value()) {
      const auto &val = res.value();

      const double orig_mb =
          static_cast<double>(val.original_size_bytes) / bytes_per_mb;
      const double comp_mb =
          static_cast<double>(val.compressed_size_bytes) / bytes_per_mb;
      const double reduction =
          100.0 * (1.0 - (static_cast<double>(val.compressed_size_bytes) /
                          static_cast<double>(val.original_size_bytes)));

      spdlog::info(
          "[OK] {} | {:.2f} MB -> {:.2f} MB ({:.1f}% saved) in {:.1f} ms",
          val.original_path.filename().string(), orig_mb, comp_mb, reduction,
          val.time_taken_ms);

      total_orig_bytes += val.original_size_bytes;
      total_comp_bytes += val.compressed_size_bytes;
      success_count++;
    } else {
      spdlog::error("[FAILED] {}", to_string(res.error()));
    }
  }

  if (success_count > 0 && total_orig_bytes > 0) {
    const double total_orig_mb =
        static_cast<double>(total_orig_bytes) / bytes_per_mb;
    const double total_comp_mb =
        static_cast<double>(total_comp_bytes) / bytes_per_mb;
    const double total_reduction =
        100.0 * (1.0 - (static_cast<double>(total_comp_bytes) /
                        static_cast<double>(total_orig_bytes)));

    spdlog::info("--------------------------------------------------");
    spdlog::info("Processing completed!");
    spdlog::info("Converted images: {}/{}", success_count,
                 images_to_process.size());
    spdlog::info("Total initial size: {:.2f} MB", total_orig_mb);
    spdlog::info("Total final size:   {:.2f} MB", total_comp_mb);
    spdlog::info("Total savings:      {:.2f}%", total_reduction);
  }

  return 0;
}
