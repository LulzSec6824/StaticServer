#!/bin/bash

# StaticServer Project Build Script
# Handles compilation, testing, and running of the StaticServer project

set -e  # Exit on any error

# Colors for better output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Default configuration
BUILD_TYPE="Release"
BUILD_DIR="build"
BUILD_TESTS=0
CLEAN=0
RUN=0
DEBUG=0
PGO_MODE=""
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2) # Try to detect CPU cores

# Function to print usage information
print_usage() {
    echo -e "${BLUE}StaticServer Build Script${NC}"
    echo -e "Usage: $0 [options]"
    echo -e ""
    echo -e "Options:"
    echo -e "  ${YELLOW}-h, --help${NC}       Show this help message"
    echo -e "  ${YELLOW}-c, --clean${NC}      Clean the build directory"
    echo -e "  ${YELLOW}-t, --test${NC}       Build and run tests"
    echo -e "  ${YELLOW}-r, --run${NC}        Run the server after building"
    echo -e "  ${YELLOW}-d, --debug${NC}      Build in debug mode"
    echo -e "  ${YELLOW}-j, --jobs N${NC}     Set number of parallel build jobs (default: autodetect)"
    echo -e "  ${YELLOW}--pgo-generate${NC}   Build with PGO profile generation"
    echo -e "  ${YELLOW}--pgo-use${NC}        Build using PGO profiles (run after generate and using the server)"
    echo -e ""
    echo -e "Examples:"
    echo -e "  $0                 # Build the project with optimizations"
    echo -e "  $0 --clean         # Clean and rebuild"
    echo -e "  $0 --debug --test  # Build in debug mode and run tests"
    echo -e "  $0 --pgo-generate  # Build with PGO instrumentation"
    echo -e ""
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -t|--test)
            BUILD_TESTS=1
            shift
            ;;
        -r|--run)
            RUN=1
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            DEBUG=1
            shift
            ;;
        -j|--jobs)
            if [[ $2 =~ ^[0-9]+$ ]]; then
                JOBS=$2
                shift 2
            else
                echo -e "${RED}Error: --jobs requires a number argument${NC}"
                exit 1
            fi
            ;;
        --pgo-generate)
            PGO_MODE="generate"
            shift
            ;;
        --pgo-use)
            PGO_MODE="use"
            shift
            ;;
        *)
            echo -e "${RED}Error: Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
done

# Handle first-time setup (Makefile removal)
if [ -f Makefile ]; then
    echo -e "${YELLOW}=== Migrating from Makefile to CMake ===${NC}"
    echo -e "Creating backup of original Makefile..."
    cp Makefile Makefile.bak
    echo -e "Backup created as Makefile.bak"
    
    rm Makefile
    echo -e "Original Makefile removed"
    
    echo -e "Cleaning up old build artifacts..."
    rm -rf obj bin static_server static_server_debug
    rm -f tests/test_* test_temp_file.txt empty_file.txt large_file.txt
fi

# Clean if requested
if [ $CLEAN -eq 1 ]; then
    echo -e "${YELLOW}=== Cleaning Build Directory ===${NC}"
    rm -rf ${BUILD_DIR}
fi

# Create build directory if it doesn't exist
if [ ! -d "${BUILD_DIR}" ]; then
    echo -e "${BLUE}Creating build directory...${NC}"
    mkdir -p ${BUILD_DIR}
fi

# Enter build directory
cd ${BUILD_DIR}

# Configure CMake with appropriate options
echo -e "${YELLOW}=== Configuring CMake (${BUILD_TYPE} mode) ===${NC}"

CMAKE_OPTIONS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"

# Add test flag if tests are requested
if [ $BUILD_TESTS -eq 1 ]; then
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DBUILD_TESTS=ON"
fi

# Add PGO flags if requested
if [ "$PGO_MODE" = "generate" ]; then
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DENABLE_PGO_GENERATE=ON"
    echo -e "${YELLOW}Enabling PGO profile generation${NC}"
elif [ "$PGO_MODE" = "use" ]; then
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DENABLE_PGO_USE=ON"
    echo -e "${YELLOW}Using PGO profiles for optimization${NC}"
fi

# Run CMake configuration
echo -e "Running: cmake ${CMAKE_OPTIONS} .."
cmake ${CMAKE_OPTIONS} ..

# Build the project
echo -e "${YELLOW}=== Building Project (using ${JOBS} parallel jobs) ===${NC}"
cmake --build . --config ${BUILD_TYPE} -j ${JOBS}

# Run tests if requested
if [ $BUILD_TESTS -eq 1 ]; then
    echo -e "${YELLOW}=== Running Tests ===${NC}"
    ctest -j ${JOBS} --output-on-failure
    
    # Check test results
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
    else
        echo -e "${RED}Some tests failed${NC}"
        exit 1
    fi
fi

# Go back to project root
cd ..

# Run the server if requested
if [ $RUN -eq 1 ]; then
    echo -e "${YELLOW}=== Running StaticServer ===${NC}"
    if [ $DEBUG -eq 1 ]; then
        echo -e "Starting debug version of server..."
        ./build/bin/static_server_debug
    else
        echo -e "Starting optimized server..."
        ./build/bin/static_server
    fi
fi

# Print binary size for release builds (informational)
if [ "${BUILD_TYPE}" = "Release" ] && [ -f ./build/bin/static_server ]; then
    BINARY_SIZE=$(ls -lh ./build/bin/static_server | awk '{print $5}')
    echo -e "${GREEN}=== Build Successful! Binary size: ${BINARY_SIZE} ===${NC}"
else
    echo -e "${GREEN}=== Build Successful! ===${NC}"
fi

# Provide guidance for PGO if in generate mode
if [ "$PGO_MODE" = "generate" ]; then
    echo -e ""
    echo -e "${YELLOW}PGO Profile Generation:${NC}"
    echo -e "1. Run the program to generate profile data:"
    echo -e "   ${BLUE}./build/bin/static_server${NC}"
    echo -e "2. After exercising the program, rebuild with profiles:"
    echo -e "   ${BLUE}./build.sh --pgo-use${NC}"
    echo -e ""
fi

exit 0