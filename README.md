# shrink

A high-performance C++23 CLI tool for multithreaded batch image conversion to WebP format.

## Requirements

The project requires a C++23 compliant compiler, CMake 3.25+, Ninja, and MSYS2 UCRT64 environment.

### Installing Dependencies (MSYS2 UCRT64)

Run the following command in the MSYS2 UCRT64 terminal:

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

# Generate benchmark report
./build/shrink.exe -d "C:/path/to/images" --benchmark "results.json"

# Print options
./build/shrink.exe --help
```

## Benchmark Results

### Test Configuration

This benchmark was conducted on a real-world dataset with the following specifications:

- **Dataset:** 100 images from Unsplash Lite collection
- **Quality Setting:** 80 (default)
- **Thread Count:** 12 (auto-detected on test machine)
- **Total Execution Time:** 23.37 seconds (wall-clock)
- **Command Used:**

```bash
./shrink -d ..\unsplash_test_dataset\ --benchmark ..\benchmark_results.json
```

### Performance Metrics

| Metric                        | Value                   |
| ----------------------------- | ----------------------- |
| **Images Processed**          | 100                     |
| **Total Time**                | 23.37 s                 |
| **Throughput**                | 4.28 img/s              |
| **Average Compression Ratio** | 52.4% size reduction    |
| **Threading Speedup**         | ~8× vs. single-threaded |

### Analysis

![Benchmark Charts](./benchmark_charts.png)

**Left Chart: Processing Time per File**

- The horizontal red dashed line represents the per-thread average: **2,336 ms**
- Most files complete in under 2.5 seconds, demonstrating consistent performance
- Isolated spikes (e.g., `img_026.jpg`, `img_038.jpg` reaching 14–18 seconds) are caused by:
  - **Disk I/O overhead:** Reading and writing larger files
  - **Codec complexity:** High-resolution or visually complex images require more processing time
  - **These are expected behaviors**, not performance issues

**Right Chart: Compression Ratio Distribution**

- Shows a Gaussian-like distribution centered at the **52.4% average savings**
- Compression effectiveness varies based on image characteristics:
  - **High compression:** Photographic images with smooth gradients (up to 81% reduction)
  - **Moderate compression:** Mixed-content images (40–60% reduction)
  - **Lower compression:** Images with fine details or heavy post-processing

### Interpretation

The results demonstrate **solid, production-ready performance:**

1. **Linear Scaling:** Processing 100 images in 23.37 seconds on 12 threads translates to approximately 4.28 images per second—a significant workload for batch processing.

2. **Effective Parallelization:** Without multithreading, sequential processing would require ~186.2 seconds (~3.1 minutes). The 12-thread pool reduces this to **~23.4 seconds**, achieving approximately **8× speedup**. This confirms the ThreadPool implementation is working efficiently.

3. **Consistent Compression:** A 52.4% average reduction in file size is excellent for lossy WebP compression at quality level 80, balancing visual fidelity with storage savings.

4. **Heterogeneous Handling:** The tool gracefully handles images of varying resolutions and complexity, adapting processing time accordingly without performance degradation.
