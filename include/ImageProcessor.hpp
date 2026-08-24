#ifndef IMAGE_PROCESSOR_HPP
#define IMAGE_PROCESSOR_HPP

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string_view>

namespace fs = std::filesystem;

/** @brief Failure categories returned by the image compression pipeline. */
enum class CompressionError {
  FileNotFound,
  InvalidImage,
  EncodingFailed,
  WriteFailed,
  ReadError,
  DecoderError
};

/**
 * @brief Measurements and paths produced by one successful compression.
 */
struct CompressionResult {
  /** @brief Source image path. */
  fs::path original_path;
  /** @brief Generated WebP path. */
  fs::path output_path;
  /** @brief Source file size in bytes. */
  uint64_t original_size_bytes;
  /** @brief Generated WebP size in bytes. */
  uint64_t compressed_size_bytes;
  /** @brief End-to-end compression duration in milliseconds. */
  double time_taken_ms;
  /** @brief Decoded image width in pixels. */
  int width{0};
  /** @brief Decoded image height in pixels. */
  int height{0};
};

/**
 * @brief Converts a compression error to a human-readable message.
 * @param err Error value to describe.
 * @return A stable, user-facing error message.
 */
std::string_view to_string(CompressionError err);

/** @brief Provides the image decoding, resizing, and WebP encoding pipeline. */
class ImageProcessor {
public:
  /**
   * @brief Compresses an image to a lossy WebP file beside the source file.
   * @param input_path Source PNG, JPG, JPEG, or BMP path.
   * @param quality WebP quality from 0.0 (smallest) to 100.0 (highest).
   * @param max_width Optional maximum width; aspect ratio is preserved.
   * @param max_height Optional maximum height; aspect ratio is preserved.
   * @return Compression metrics, or the reason the operation failed.
   */
  static std::expected<CompressionResult, CompressionError>
  compress_to_webp(const fs::path &input_path, float quality = 80.0f,
                   std::optional<int> max_width = std::nullopt,
                   std::optional<int> max_height = std::nullopt);
};

#endif
