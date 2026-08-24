cmake_minimum_required(VERSION 3.25)
project(shrink LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configurazione Standard C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# FetchContent per gestire TUTTE le dipendenze esterne
# ============================================================================
include(FetchContent)

# ---- libwebp ----
# Build libwebp da sorgenti con opzioni ottimizzate
FetchContent_Declare(
    libwebp
    GIT_REPOSITORY https://github.com/webmproject/libwebp.git
    GIT_TAG        v1.3.2
    PATCH_COMMAND ${CMAKE_COMMAND} -E echo "libwebp declared"
)

FetchContent_MakeAvailable(libwebp)

# ---- cxxopts (header-only) ----
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        v3.1.1
)
FetchContent_MakeAvailable(cxxopts)

# ---- spdlog ----
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# ---- stb (header-only, system include per evitare warning) ----
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    SYSTEM
)
FetchContent_MakeAvailable(stb)

# ---- indicators (header-only, progress bar) ----
FetchContent_Declare(
    indicators
    GIT_REPOSITORY https://github.com/p-ranav/indicators.git
    GIT_TAG        v2.3
)
FetchContent_MakeAvailable(indicators)

# ============================================================================
# Target per implementazione STB
# ============================================================================
# Creiamo un file temporaneo con le macro d'implementazione di stb
set(STB_WRAPPER_SRC "${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp")
file(WRITE "${STB_WRAPPER_SRC}" [[
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
]])

add_library(stb_impl STATIC "${STB_WRAPPER_SRC}")
target_include_directories(stb_impl SYSTEM PUBLIC ${stb_SOURCE_DIR})

# Disabilita warning per stb (codice di terze parti)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(stb_impl PRIVATE -w)
endif()

if(MSVC)
  target_compile_options(stb_impl PRIVATE /W0)
endif()

# ============================================================================
# Target Principale: shrink
# ============================================================================
add_executable(shrink
    src/main.cpp
    src/Application.cpp
    src/ImageProcessor.cpp
)

target_include_directories(shrink PRIVATE include)

# ============================================================================
# Compiler Flags - Warning rigorosi SOLO per il codice del progetto
# ============================================================================

# GCC / Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(shrink PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Werror
    -fdiagnostics-color=always
  )

  # Ottimizzazioni per Release
  target_compile_options(shrink PRIVATE
    $<$<CONFIG:Release>:-march=native -O3>
  )

  # Link statico per MSYS2 / MinGW
  if(MINGW OR MSYS)
    target_link_options(shrink PRIVATE
      -static-libgcc
      -static-libstdc++
      -lstdc++exp
    )
  endif()
endif()

# MSVC
if(MSVC)
  target_compile_options(shrink PRIVATE
    /W4
    /WX
    /permissive-
    /std:c++latest
  )

  # Ottimizzazioni per Release
  target_compile_options(shrink PRIVATE
    $<$<CONFIG:Release>:/O2 /Oi>
  )
endif()

# Clang-cl (Clang compilato con front-end MSVC)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND MSVC)
  target_compile_options(shrink PRIVATE
    -Wall
    -Wextra
    -Werror
  )
endif()

# ============================================================================
# Linking delle Dipendenze
# ============================================================================
target_link_libraries(shrink PRIVATE
    stb_impl
    cxxopts::cxxopts
    spdlog::spdlog
    webp
    indicators::indicators
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(shrink PRIVATE -Wno-dangling-reference)
endif()

# ============================================================================
# Post-Build (Opzionale): Copia configurazione di debug
# ============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_custom_command(TARGET shrink POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Build completed successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "Executable: $<TARGET_FILE:shrink>"
  )
endif()