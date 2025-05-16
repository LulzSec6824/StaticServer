# StaticServer Build Instructions

This document explains how to build and run the StaticServer project using the `build.sh` script.

## Quick Start

To build the project with default optimizations:

```bash
./build.sh
```

This will compile the project in Release mode with all optimizations enabled.

## Build Script Options

The `build.sh` script provides several options to customize the build process:

```
Options:
  -h, --help       Show this help message
  -c, --clean      Clean the build directory
  -t, --test       Build and run tests
  -r, --run        Run the server after building
  -d, --debug      Build in debug mode
  -j, --jobs N     Set number of parallel build jobs (default: autodetect)
  --pgo-generate   Build with PGO profile generation
  --pgo-use        Build using PGO profiles
```

## Common Build Scenarios

### Clean and Rebuild

To clean the build directory and recompile:

```bash
./build.sh --clean
```

### Debug Build

For development and debugging:

```bash
./build.sh --debug
```

### Build and Run Tests

To build the project and run its test suite:

```bash
./build.sh --test
```

### Build and Run the Server

To compile and immediately start the server:

```bash
./build.sh --run
```

### Optimized Build with Profile-Guided Optimization (PGO)

PGO can significantly improve performance by optimizing based on actual usage patterns. This is a two-step process:

Step 1: Generate profile data:
```bash
./build.sh --pgo-generate --run
# Use the server with real-world traffic/patterns
```

Step 2: Build with the profile data:
```bash
./build.sh --pgo-use
```

The resulting binary will be optimized based on the observed usage patterns.

## Advanced Options

### Build with Multiple CPU Cores

Specify the number of parallel build jobs:

```bash
./build.sh --jobs 4
```

### Debug Build with Tests

```bash
./build.sh --debug --test
```

### Combining Multiple Options

You can combine multiple options:

```bash
./build.sh --clean --debug --test --run
```

## Manual CMake Commands

If you prefer to use CMake directly:

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## System Requirements

- CMake 3.10 or newer
- C++11 compatible compiler
- Bash shell
- pthread library (on Unix systems)