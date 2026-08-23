# shrink

A high-performance C++23 CLI tool for multithreaded batch image conversion to WebP format.

## Requirements

The project requires a C++23 compliant compiler, CMake 3.25+, Ninja, and MSYS2 UCRT64 environment.

### Installing Dependencies

You need to install the following dependencies:
- cmake
- ninja
- libwebp
- cxxopts
- spdlog

On Windows you can install theme with the help of the MSYS2 UCRT64 terminal:

```bash
pacman -S --needed \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-libwebp \
    mingw-w64-ucrt-x86_64-cxxopts \
    mingw-w64-ucrt-x86_64-spdlog \
    git
```

## Building the Project

1. Clone the repository:

```bash
git clone <repository_url>
cd shrink
```

2. Configure CMake:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

3. Build:

```bash
cmake --build build
```

The output executable `shrink.exe` will be located in the `build/` directory.

## Usage

```bash
# Basic conversion (default quality 80, auto thread detection)
./build/shrink.exe -d "C:/path/to/images"

# Custom quality (e.g., 75.0) and explicit thread count
./build/shrink.exe -d "C:/path/to/images" -q 75.0 -t 4

# Print options
./build/shrink.exe --help
```

## Options

- `-d, --directory <path>`: Path to the directory containing images (required).
- `-q, --quality <float>`: Compression quality from 0.0 to 100.0 (default: 80.0).
- `-t, --threads <size_t>`: Number of threads to use, 0 for auto-detect (default: 0).
- `-h, --help`: Display help message.
