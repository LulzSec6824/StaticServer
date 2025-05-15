# Compiler and compiler flags
CXX = clang++
# Common flags
COMMON_FLAGS = -Wall -Werror -I include
# Debug build flags
DEBUG_FLAGS = -std=c++11 -g -D_DEBUG $(COMMON_FLAGS)
# Release build flags (optimized for size and performance)
RELEASE_FLAGS = -std=c++11 -O3 -march=native -flto -ffunction-sections -fdata-sections $(COMMON_FLAGS)
# Strip unused sections during linking (release mode)
RELEASE_LDFLAGS = -pthread -Wl,--gc-sections -flto
# Debug linking
DEBUG_LDFLAGS = -pthread

# Source files and output
SRC_DIR = src
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_DIR = obj
OBJ_DIR_DEBUG = $(OBJ_DIR)/debug
OBJ_DIR_RELEASE = $(OBJ_DIR)/release
OBJ_FILES_DEBUG = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR_DEBUG)/%.o,$(SRC_FILES))
OBJ_FILES_RELEASE = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR_RELEASE)/%.o,$(SRC_FILES))
MAIN_OBJ_DEBUG = $(OBJ_DIR_DEBUG)/main.o
MAIN_OBJ_RELEASE = $(OBJ_DIR_RELEASE)/main.o
SERVER_OBJS_DEBUG = $(filter-out $(MAIN_OBJ_DEBUG),$(OBJ_FILES_DEBUG))
SERVER_OBJS_RELEASE = $(filter-out $(MAIN_OBJ_RELEASE),$(OBJ_FILES_RELEASE))
EXECUTABLE = static_server
EXECUTABLE_DEBUG = static_server_debug

# Test configuration
TEST_DIR = tests
TEST_SRC = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJ = $(patsubst $(TEST_DIR)/%.cpp,$(OBJ_DIR_DEBUG)/$(TEST_DIR)/%.o,$(TEST_SRC))
TEST_EXEC = $(patsubst $(TEST_DIR)/%.cpp,$(TEST_DIR)/%,$(TEST_SRC))
TEST_UTILS_H = $(TEST_DIR)/test_utils.hpp

# Default target
all: release

# Make directories
$(OBJ_DIR_DEBUG) $(OBJ_DIR_RELEASE) $(OBJ_DIR_DEBUG)/$(TEST_DIR):
	@mkdir -p $@

# Debug target
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: LDFLAGS = $(DEBUG_LDFLAGS)
debug: $(OBJ_DIR_DEBUG) $(EXECUTABLE_DEBUG)

# Release target (optimized for performance)
release: CXXFLAGS = $(RELEASE_FLAGS)
release: LDFLAGS = $(RELEASE_LDFLAGS)
release: $(OBJ_DIR_RELEASE) $(EXECUTABLE)

# Link object files to create debug executable
$(EXECUTABLE_DEBUG): $(OBJ_FILES_DEBUG)
	@echo "Linking debug build $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Debug build complete: $@"

# Link object files to create release executable
$(EXECUTABLE): $(OBJ_FILES_RELEASE)
	@echo "Linking release build $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Release build complete: $@"
	@echo "Release binary size: $$(du -h $@ | cut -f1)"

# Compile source files to debug object files
$(OBJ_DIR_DEBUG)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR_DEBUG)
	@echo "Compiling debug $<..."
	$(CXX) $(DEBUG_FLAGS) -c -o $@ $<

# Compile source files to release object files
$(OBJ_DIR_RELEASE)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR_RELEASE)
	@echo "Compiling release $<..."
	$(CXX) $(RELEASE_FLAGS) -c -o $@ $<

# Test targets
test_dir:
	@mkdir -p $(TEST_DIR) $(OBJ_DIR_DEBUG)/$(TEST_DIR)
	@mkdir -p public

# Make sure test_utils.hpp exists before compiling tests
$(OBJ_DIR_DEBUG)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp $(TEST_UTILS_H) | $(OBJ_DIR_DEBUG)/$(TEST_DIR)
	@echo "Compiling test $<..."
	$(CXX) $(DEBUG_FLAGS) -c -o $@ $<

# Link test object files with server object files
$(TEST_DIR)/%: $(OBJ_DIR_DEBUG)/$(TEST_DIR)/%.o $(SERVER_OBJS_DEBUG)
	@echo "Linking test $@..."
	$(CXX) $(DEBUG_FLAGS) -o $@ $^ $(DEBUG_LDFLAGS)

# Run all tests
tests: test_dir $(TEST_EXEC)
	@echo "\n===== Running All Tests ====="
	@failed=0; \
	for test in $(TEST_EXEC); do \
		echo "\n===== Running $$test ====="; \
		if ! ./$$test; then \
			echo "\n\033[1;31m===== Test $$test FAILED =====\033[0m\n"; \
			failed=$$((failed + 1)); \
		else \
			echo "\n\033[1;32m===== Test $$test PASSED =====\033[0m\n"; \
		fi; \
	done; \
	if [ $$failed -eq 0 ]; then \
		echo "\n\033[1;32m===== All Tests Passed! =====\033[0m\n"; \
	else \
		echo "\n\033[1;31m===== $$failed Test(s) Failed =====\033[0m\n"; \
		exit 1; \
	fi

# Individual test targets
test_config: $(TEST_DIR)/test_config
	@echo "Running config tests..."
	@./$(TEST_DIR)/test_config

test_file_utils: $(TEST_DIR)/test_file_utils
	@echo "Running file utilities tests..."
	@./$(TEST_DIR)/test_file_utils

test_server: $(TEST_DIR)/test_server
	@echo "Running server tests..."
	@./$(TEST_DIR)/test_server

test_integration: $(TEST_DIR)/test_integration
	@echo "Running integration tests..."
	@./$(TEST_DIR)/test_integration

# Build all tests without running them
build_tests: $(TEST_EXEC)
	@echo "All tests built successfully"

# Clean target
clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DIR)
	@rm -f $(EXECUTABLE) $(EXECUTABLE_DEBUG) $(TEST_EXEC)
	@rm -f test_temp_file.txt empty_file.txt large_file.txt
	@echo "Cleanup complete"

# Run target (uses optimized version by default)
run: $(EXECUTABLE)
	@echo "Running optimized server..."
	@./$(EXECUTABLE)

# Run debug version
run_debug: $(EXECUTABLE_DEBUG)
	@echo "Running debug server..."
	@./$(EXECUTABLE_DEBUG)

# Help target for documentation
help:
	@echo "Available targets:"
	@echo "  all            - Build the optimized server executable (same as 'release')"
	@echo "  debug          - Build the server with debug symbols"
	@echo "  release        - Build the server with optimizations for size and speed"
	@echo "  tests          - Build and run all tests"
	@echo "  test_config    - Run just the config tests"
	@echo "  test_file_utils - Run just the file utility tests"
	@echo "  test_server    - Run just the server tests"
	@echo "  test_integration - Run just the integration tests"
	@echo "  build_tests    - Build all tests without running them"
	@echo "  clean          - Remove all built files"
	@echo "  run            - Run the optimized server"
	@echo "  run_debug      - Run the debug server"
	@echo "  help           - Display this help message"

.PHONY: all debug release clean run run_debug tests test_dir test_config test_file_utils test_server test_integration build_tests help
