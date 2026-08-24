#include "../include/Application.hpp"
#include "../include/ThreadPool.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>

#include <spdlog/spdlog.h>

/**
 * @brief Initializes the application and discovers its input images.
 * @param config Parsed command-line configuration.
 */
Application::Application(Config config) : cfg_(std::move(config)) {
  collect_images();
}

/** @copydoc Application::is_supported_image */
bool Application::is_supported_image(const fs::path &path) {
  auto ext = path.extension().string();
  for (auto &c : ext)
    c = static_cast<char>(std::tolower(c));
  return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp";
}

/** @copydoc Application::collect_images */
void Application::collect_images() {
  if (cfg_.is_single_file) {
    if (fs::exists(cfg_.target_path) && fs::is_regular_file(cfg_.target_path) &&
        is_supported_image(cfg_.target_path))
      image_to_process_.push_back(cfg_.target_path);
    return;
  }

  if (!fs::exists(cfg_.target_path) || !fs::is_directory(cfg_.target_path))
    return;

  auto is_supported_entry = [](const fs::directory_entry &entry) {
    return entry.is_regular_file() && is_supported_image(entry.path());
  };

  for (const auto &entry : fs::directory_iterator(cfg_.target_path) |
                               std::views::filter(is_supported_entry)) {
    image_to_process_.push_back(entry.path());
  }
}

/** @copydoc Application::run */
int Application::run() {
  if (image_to_process_.empty()) {
    spdlog::warn(
        "No supported images (.png, .jpg, .jpeg, .bmp) found to process.");
    return 0;
  }

  spdlog::info("Found {} image(s). Starting processing with {} thread(s) "
               "(Quality: {})...",
               image_to_process_.size(), cfg_.threads_count, cfg_.quality);

  // This measures the complete batch, including worker startup and result
  // collection.
  const auto start_time = std::chrono::high_resolution_clock::now();

  ThreadPool pool(cfg_.threads_count);
  std::vector<std::future<std::expected<CompressionResult, CompressionError>>>
      futures;
  futures.reserve(image_to_process_.size());

  for (const auto &img_path : image_to_process_) {
    futures.push_back(pool.enqueue([img_path, cfg = cfg_]() {
      auto res = ImageProcessor::compress_to_webp(
          img_path, cfg.quality, cfg.max_width, cfg.max_height);
      return res;
    }));
  }

  uint64_t total_orig_bytes = 0;
  uint64_t total_comp_bytes = 0;
  size_t success_count = 0;
  constexpr double bytes_per_mb = 1024.0 * 1024.0;

  std::vector<CompressionResult> successful_results;
  successful_results.reserve(image_to_process_.size());

  // Futures are consumed in submission order; each future still represents a
  // task that may have completed earlier on any worker.
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

      const std::string filename_str = val.original_path.filename().string();
      spdlog::info(
          "[OK] {} | {:.2f} MB -> {:.2f} MB ({:.1f}% saved) in {:.1f} ms",
          filename_str, orig_mb, comp_mb, reduction, val.time_taken_ms);

      total_orig_bytes += val.original_size_bytes;
      total_comp_bytes += val.compressed_size_bytes;
      success_count++;
      successful_results.push_back(val);
    } else {
      const auto err_msg = to_string(res.error());
      spdlog::error("[FAILED] {}", err_msg);
    }
  }

  const auto end_time = std::chrono::high_resolution_clock::now();
  const auto duration_ms =
      std::chrono::duration<double, std::milli>(end_time - start_time).count();

  print_summary(success_count, image_to_process_.size(), total_orig_bytes,
                total_comp_bytes);

  if (cfg_.benchmark_export_path.has_value()) {
    export_benchmark_json(*cfg_.benchmark_export_path, successful_results,
                          duration_ms, cfg_.threads_count);
  }

  return 0;
}

/** @copydoc Application::print_summary */
void Application::print_summary(size_t success_count, size_t total_count,
                                uint64_t orig_bytes, uint64_t comp_bytes) {
  if (success_count == 0 || orig_bytes == 0)
    return;

  constexpr double bytes_per_mb = 1024.0 * 1024.0;
  const double total_orig_mb = static_cast<double>(orig_bytes) / bytes_per_mb;
  const double total_comp_mb = static_cast<double>(comp_bytes) / bytes_per_mb;
  const double total_reduction =
      100.0 * (1.0 - (static_cast<double>(comp_bytes) /
                      static_cast<double>(orig_bytes)));

  spdlog::info("--------------------------------------------------");
  spdlog::info("Processing completed!");
  spdlog::info("Converted images: {}/{}", success_count, total_count);
  spdlog::info("Total initial size: {:.2f} MB", total_orig_mb);
  spdlog::info("Total final size:   {:.2f} MB", total_comp_mb);
  spdlog::info("Total savings:      {:.2f}%", total_reduction);
}

/** @copydoc Application::export_benchmark_json */
void Application::export_benchmark_json(
    const fs::path &export_path, const std::vector<CompressionResult> &results,
    double total_duration_ms, size_t threads_count) {
  std::ofstream file(export_path);
  if (!file.is_open()) {
    spdlog::error("Failed to open benchmark export file: {}",
                  export_path.string());
    return;
  }

  file << "{\n";
  file << "  \"summary\": {\n";
  file << "    \"threads\": " << threads_count << ",\n";
  file << "    \"total_time_ms\": " << total_duration_ms << ",\n";
  file << "    \"total_files\": " << results.size() << "\n";
  file << "  },\n";
  file << "  \"results\": [\n";

  for (size_t i = 0; i < results.size(); ++i) {
    const auto &r = results[i];
    const double savings =
        100.0 * (1.0 - (static_cast<double>(r.compressed_size_bytes) /
                        static_cast<double>(r.original_size_bytes)));

    file << "    {\n";
    file << "      \"file\": \"" << r.original_path.filename().string()
         << "\",\n";
    file << "      \"original_bytes\": " << r.original_size_bytes << ",\n";
    file << "      \"compressed_bytes\": " << r.compressed_size_bytes << ",\n";
    file << "      \"savings_percent\": " << savings << ",\n";
    file << "      \"time_ms\": " << r.time_taken_ms << "\n";
    file << "    }" << (i + 1 < results.size() ? "," : "") << "\n";
  }

  file << "  ]\n";
  file << "}\n";
  spdlog::info("Benchmark report saved to {}", export_path.string());
}
