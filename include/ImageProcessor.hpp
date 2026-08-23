#ifndef IMAGE_PROCESSOR_HPP
#define IMAGE_PROCESSOR_HPP

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string_view>

namespace fs = std::filesystem;

enum class CompressionError {
  FileNotFound,
  InvalidImage,
  EncodingFailed,
  WriteFailed
};

struct CompressionResult {
  fs::path original_path;
  fs::path output_path;
  uint64_t original_size_bytes;
  uint64_t compressed_size_bytes;
  double time_taken_ms;
};

std::string_view to_string(CompressionError err);

class ImageProcessor {
public:
  static std::expected<CompressionResult, CompressionError>
  compress_to_webp(const fs::path &input_path, float quality = 80.0f);
};

#endif
