cmake_minimum_required(VERSION 3.25)
project(ImgCompressor LANGUAGES CXX C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dipendenze MSYS2 UCRT64
find_package(cxxopts REQUIRED)
find_package(spdlog REQUIRED)
find_package(WebP REQUIRED)


# Per stb_image scarichiamo l'header se non presente localmente
include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

include_directories(include ${stb_SOURCE_DIR})

add_executable(shrink
    src/main.cpp
    src/ImageProcessor.cpp
)

target_compile_options(shrink PRIVATE
    # Applicato solo se il compilatore è GCC o Clang
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

target_link_libraries(shrink PRIVATE
    cxxopts::cxxopts
    spdlog::spdlog
    WebP::webp
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
  target_link_libraries(shrink PRIVATE stdc++exp)
endif()
