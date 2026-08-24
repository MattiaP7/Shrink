#include <filesystem>
#include <ranges>
#include <vector>

#include <spdlog/spdlog.h>

#include "../include/Config.hpp"
#include "../include/ImageProcessor.hpp"
#include "../include/ThreadPool.hpp"

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
  auto config_opt = Config::parse(argc, argv);
  if (!config_opt.has_value()) {
    return 0; // Help stampato o errore nei flag
  }

  const Config cfg = *config_opt;

  std::vector<fs::path> images_to_process;

  auto is_supported_image = [](const fs::path &path) {
    auto ext = path.extension().string();
    for (auto &c : ext) {
      c = static_cast<char>(std::tolower(c));
    }
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
  };

  if (cfg.is_single_file) {
    if (!fs::exists(cfg.target_path) || !fs::is_regular_file(cfg.target_path)) {
      spdlog::error("Error: The path '{}' is not a valid file.",
                    cfg.target_path.string());
      return 1;
    }
    if (!is_supported_image(cfg.target_path)) {
      spdlog::error("Error: Unsupported image extension for file '{}'.",
                    cfg.target_path.string());
      return 1;
    }
    images_to_process.push_back(cfg.target_path);
  } else {
    if (!fs::exists(cfg.target_path) || !fs::is_directory(cfg.target_path)) {
      spdlog::error("Error: The path '{}' is not a valid directory.",
                    cfg.target_path.string());
      return 1;
    }

    auto is_supported_entry = [&](const fs::directory_entry &entry) {
      return entry.is_regular_file() && is_supported_image(entry.path());
    };

    for (const auto &entry : fs::directory_iterator(cfg.target_path) |
                                 std::views::filter(is_supported_entry)) {
      images_to_process.push_back(entry.path());
    }
  }

  if (images_to_process.empty()) {
    spdlog::warn(
        "No supported images (.png, .jpg, .jpeg, .bmp) found to process.");
    return 0;
  }

  spdlog::info("Found {} image(s). Starting processing with {} thread(s) "
               "(Quality: {})...",
               images_to_process.size(), cfg.threads_count, cfg.quality);

  ThreadPool pool(cfg.threads_count);
  std::vector<std::future<std::expected<CompressionResult, CompressionError>>>
      futures;

  for (const auto &img_path : images_to_process) {
    futures.push_back(pool.enqueue([img_path, cfg]() {
      return ImageProcessor::compress_to_webp(img_path, cfg.quality,
                                              cfg.max_width, cfg.max_height);
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