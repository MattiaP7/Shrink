cmake_minimum_required(VERSION 3.25)
project(ImgCompressor LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configurazione Standard C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dipendenze MSYS2 UCRT64
find_package(cxxopts REQUIRED)
find_package(spdlog REQUIRED)
find_package(WebP REQUIRED)

# Gestione libreria header-only stb via FetchContent
include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    SYSTEM
)
FetchContent_MakeAvailable(stb)

# --- TARGET DEDICATO PER L'IMPLEMENTAZIONE DI STB ---
# Generiamo un file sorgente temporaneo per le macro d'implementazione
set(STB_WRAPPER_SRC "${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp")
file(WRITE "${STB_WRAPPER_SRC}" [[
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
]])

add_library(stb_impl STATIC "${STB_WRAPPER_SRC}")
target_include_directories(stb_impl SYSTEM PUBLIC ${stb_SOURCE_DIR})

# Disabilitiamo i warning aggressivi ESCLUSIVAMENTE per il target stb_impl
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(stb_impl PRIVATE
        -w # Disabilita tutti i warning per l'implementazione stb
    )
endif()
# ---------------------------------------------------

# Definizione Eseguibile Principale
add_executable(shrink
    src/main.cpp
    src/ImageProcessor.cpp
)

target_include_directories(shrink PRIVATE include)

# Warning rigorosi applicati SOLO al tuo codice (shrink)
target_compile_options(shrink PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Werror
        -fdiagnostics-color=always
    >
)

target_compile_options(shrink PRIVATE
    $<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:
        -march=native
    >
)

# Linking Librerie
target_link_libraries(shrink PRIVATE
    stb_impl
    cxxopts::cxxopts
    spdlog::spdlog
    WebP::webp
)

# Nota: -lstdc++exp va passato tra le link_options/libraries, non tra le compile_options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(shrink PRIVATE -static-libgcc -static-libstdc++ -lstdc++exp)
endif()
