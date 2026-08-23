#include "../include/ImageProcessor.hpp"
#include <chrono>
#include <fstream>

// Disabilitiamo temporaneamente i warning per la libreria esterna stb_image
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma GCC diagnostic pop

#include <webp/encode.h>

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
ImageProcessor::compress_to_webp(const fs::path &input_path, float quality) {
  if (!fs::exists(input_path)) {
    return std::unexpected(CompressionError::FileNotFound);
  }

  auto start_time = std::chrono::high_resolution_clock::now();
  uint64_t orig_size = fs::file_size(input_path);

  // 1. Decodifica immagine reale (JPG/PNG/BMP) tramite stb_image
  int width = 0, height = 0, channels = 0;
  stbi_uc *raw_pixels = stbi_load(input_path.string().c_str(), &width, &height,
                                  &channels, 4); // Forziamo RGBA (4 canali)

  if (!raw_pixels) {
    return std::unexpected(CompressionError::InvalidImage);
  }

  // 2. Codifica in WebP Lossy usando libwebp
  uint8_t *webp_data = nullptr;
  size_t webp_size =
      WebPEncodeRGBA(raw_pixels, width, height, width * 4, quality, &webp_data);

  // Libera subito la memoria pixel grezza
  stbi_image_free(raw_pixels);

  if (webp_size == 0) {
    return std::unexpected(CompressionError::EncodingFailed);
  }

  // 3. Salva file di output (.webp)
  fs::path output_path = input_path;
  output_path.replace_extension(".webp");

  std::ofstream out_file(output_path, std::ios::binary);
  if (!out_file) {
    WebPFree(webp_data);
    return std::unexpected(CompressionError::WriteFailed);
  }

  out_file.write(reinterpret_cast<const char *>(webp_data),
                 static_cast<long long>(webp_size));
  WebPFree(webp_data);

  auto end_time = std::chrono::high_resolution_clock::now();
  double elapsed_ms =
      std::chrono::duration<double, std::milli>(end_time - start_time).count();

  return CompressionResult{.original_path = input_path,
                           .output_path = output_path,
                           .original_size_bytes = orig_size,
                           .compressed_size_bytes = webp_size,
                           .time_taken_ms = elapsed_ms};
}
