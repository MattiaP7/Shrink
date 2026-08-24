#include "../include/ImageProcessor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <memory>
#include <utility>
#include <vector>

#include <stb_image.h>
#include <stb_image_resize2.h>

#include <webp/encode.h>

namespace {
// Internal helper to calculate the new dimensions while maintaining the aspect
// ratio
std::pair<int, int> calculate_target_dimension(int src_w, int src_h,
                                               std::optional<int> max_w,
                                               std::optional<int> max_h) {
  if (!max_w.has_value() && !max_h.has_value())
    return {src_w, src_h};

  double scale_w =
      max_w.has_value() ? static_cast<double>(*max_w) / src_w : 1.0;
  double scale_h =
      max_h.has_value() ? static_cast<double>(*max_h) / src_h : 1.0;

  double scale = std::min(scale_w, scale_h);

  if (scale >= 1.0)
    return {src_w, src_h};

  int dst_w = static_cast<int>(std::round(src_w * scale));
  int dst_h = static_cast<int>(std::round(src_h * scale));

  return {std::max(1, dst_w), std::max(1, dst_h)};
}
} // namespace

std::string_view to_string(CompressionError err) {
  switch (err) {
  case CompressionError::FileNotFound:
    return "File non trovato";
  case CompressionError::InvalidImage:
    return "Formato immagine non valido o file corrotto";
  case CompressionError::EncodingFailed:
    return "Errore durante la codifica WebP";
  case CompressionError::WriteFailed:
    return "Impossibile scrivere il file compresso su disco";
  }
  return "Errore sconosciuto";
}

std::expected<CompressionResult, CompressionError>
ImageProcessor::compress_to_webp(const fs::path &input_path, float quality,
                                 std::optional<int> max_width,
                                 std::optional<int> max_height) {
  if (!fs::exists(input_path)) {
    return std::unexpected(CompressionError::FileNotFound);
  }

  auto start_time = std::chrono::high_resolution_clock::now();
  uint64_t orig_size = fs::file_size(input_path);

  // 1. Decodifica immagine reale (JPG/PNG/BMP) tramite stb_image
  int src_w = 0;
  int src_h = 0;
  int channels = 0;
  constexpr int desired_channels = 4;

  stbi_uc *raw_pixels = stbi_load(input_path.string().c_str(), &src_w, &src_h,
                                  &channels, desired_channels);

  if (!raw_pixels) {
    return std::unexpected(CompressionError::InvalidImage);
  }

  // RAII guard
  std::unique_ptr<stbi_uc, void (*)(void *)> stbi_guard(raw_pixels,
                                                        stbi_image_free);

  // 2 calcolo dimensioni
  auto [dst_w, dst_h] =
      calculate_target_dimension(src_w, src_h, max_width, max_height);

  const stbi_uc *final_pixels = raw_pixels;
  std::vector<stbi_uc> resized_buffer;

  // 3 resize
  if (dst_w != src_w || dst_h != src_h) {
    resized_buffer.resize(
        static_cast<size_t>(dst_w * dst_h * desired_channels));

    stbi_uc *res = stbir_resize_uint8_linear(raw_pixels, src_w, src_h, 0,
                                             resized_buffer.data(), dst_w,
                                             dst_h, 0, STBIR_RGBA);

    if (!res)
      return std::unexpected(CompressionError::EncodingFailed);

    final_pixels = resized_buffer.data();
  }

  // 4. Codifica in WebP Lossy tramite libwebp
  uint8_t *webp_data = nullptr;
  const size_t webp_size =
      WebPEncodeRGBA(final_pixels, dst_w, dst_h, dst_w * desired_channels,
                     quality, &webp_data);

  if (webp_size == 0) {
    return std::unexpected(CompressionError::EncodingFailed);
  }

  // RAII guard per la memoria allocata da WebPEncode
  std::unique_ptr<uint8_t, void (*)(void *)> webp_guard(webp_data, WebPFree);

  // 5. Scrittura file di output (.webp)
  fs::path output_path = input_path;
  output_path.replace_extension(".webp");

  std::ofstream out_file(output_path, std::ios::binary);
  if (!out_file) {
    return std::unexpected(CompressionError::WriteFailed);
  }

  out_file.write(reinterpret_cast<const char *>(webp_data),
                 static_cast<std::streamsize>(webp_size));

  if (!out_file.good()) {
    return std::unexpected(CompressionError::WriteFailed);
  }

  const auto end_time = std::chrono::high_resolution_clock::now();
  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(end_time - start_time).count();

  return CompressionResult{.original_path = input_path,
                           .output_path = output_path,
                           .original_size_bytes = orig_size,
                           .compressed_size_bytes = webp_size,
                           .time_taken_ms = elapsed_ms};
}
