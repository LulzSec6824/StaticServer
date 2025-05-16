cmake_minimum_required(VERSION 3.10)

# Project name
project(static_server VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find the number of processors for parallel builds
include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
  set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

# Enable ccache if available for faster rebuilds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# Check for support of Interprocedural Optimization (IPO/LTO)
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
  message(STATUS "IPO/LTO is supported and enabled")
else()
  message(STATUS "IPO/LTO not supported: ${IPO_ERROR}")
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Debug configuration
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -D_DEBUG")

# Release configuration with enhanced optimizations
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mtune=native -flto -ffunction-sections -fdata-sections")

# Add PGO capability for GCC and Clang
option(ENABLE_PGO_GENERATE "Enable Profile Guided Optimization (generate)" OFF)
option(ENABLE_PGO_USE "Enable Profile Guided Optimization (use)" OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Enhanced release flags
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,--gc-sections -flto")
    
    # Add PGO flags if enabled
    if(ENABLE_PGO_GENERATE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-generate")
        message(STATUS "PGO generation mode enabled")
    endif()
    
    if(ENABLE_PGO_USE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-use -fprofile-correction")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-use")
        message(STATUS "PGO use mode enabled")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC specific optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ot /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
endif()

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build: Debug or Release" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Source files
file(GLOB SOURCES "src/*.cpp")

# Add main executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Enable IPO/LTO if supported for this target
if(IPO_SUPPORTED)
  set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Debug binary gets a different name
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_debug")
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Platform-specific libraries
if(UNIX)
    # Link with pthread on Unix systems
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
    
    # Check for system optimized libraries
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
        message(STATUS "OpenMP support enabled")
    endif()
endif()

# Add precompiled headers for faster builds if supported
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
    # Identify headers that can be precompiled
    file(GLOB PCH_HEADERS "include/*.h")
    if(PCH_HEADERS)
        target_precompile_headers(${PROJECT_NAME} PRIVATE ${PCH_HEADERS})
        message(STATUS "Using precompiled headers")
    endif()
endif()

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Testing support with optimized build options
option(BUILD_TESTS "Build the tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    
    # Create test directory if it doesn't exist
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/public")
    
    # Get all test files
    file(GLOB TEST_SOURCES "tests/*.cpp")
    
    # Get server sources excluding main
    file(GLOB SERVER_SOURCES "src/*.cpp")
    list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*main.cpp$")
    
    # For each test file
    foreach(TEST_SOURCE ${TEST_SOURCES})
        # Extract the test name from the path
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
        
        # Add the executable
        add_executable(${TEST_NAME} ${TEST_SOURCE} ${SERVER_SOURCES})
        target_include_directories(${TEST_NAME} PRIVATE include tests)
        
        # Apply optimizations to tests as well
        if(IPO_SUPPORTED)
            set_property(TARGET ${TEST_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        endif()
        
        if(UNIX)
            target_link_libraries(${TEST_NAME} PRIVATE Threads::Threads)
            
            if(OpenMP_CXX_FOUND)
                target_link_libraries(${TEST_NAME} PRIVATE OpenMP::OpenMP_CXX)
            endif()
        endif()
        
        # Add the test
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()

# Post-build size optimization for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND UNIX)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Stripping binary for size optimization")
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin)

# Exclude the /pkg directory from build
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "pkg")

# Add a custom target for cleaning temporary files
add_custom_target(cleantemp
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/CMakeFiles"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/CMakeCache.txt"
    COMMAND ${CMAKE_COMMAND} -E echo "Temporary files cleaned.")

# Enhanced build instructions with optimization options
message(STATUS "")
message(STATUS "Optimized Build Instructions:")
message(STATUS "  Configure:       mkdir build && cd build && cmake ..")
message(STATUS "  Build:           cmake --build . --config Release -j${N}")
message(STATUS "  PGO Generation:  cmake -DENABLE_PGO_GENERATE=ON .. && cmake --build .")
message(STATUS "  PGO Use:         cmake -DENABLE_PGO_USE=ON .. && cmake --build .")
message(STATUS "  Debug:           cmake --build . --config Debug")
message(STATUS "  Clean:           cmake --build . --target clean")
message(STATUS "  Clean Temp:      cmake --build . --target cleantemp")
message(STATUS "  Run:             bin/${PROJECT_NAME}")
message(STATUS "  Test:            cmake -DBUILD_TESTS=ON .. && cmake --build . && ctest -j${N}")
message(STATUS "")
