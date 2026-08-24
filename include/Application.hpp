#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

#include "Config.hpp"
#include "ImageProcessor.hpp"

namespace fs = std::filesystem;

class Application {
public:
  explicit Application(Config config);

  int run();

private:
  Config cfg_;
  std::vector<fs::path> image_to_process_;

  void collect_images();
  static bool is_supported_image(const fs::path &path);
  static void print_summary(size_t success_count, size_t total_count,
                            uint64_t orig_bytes, uint64_t comp_bytes);
  static void
  export_benchmark_json(const fs::path &export_path,
                        const std::vector<CompressionResult> &results,
                        double total_duration_ms, size_t threads_count);
};

#endif
