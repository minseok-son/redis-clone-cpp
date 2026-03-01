# The name of the binary created by CMake
BINARY = server
BUILD_DIR = build

# Default: Create build folder, run CMake, and compile
all:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && make --no-print-directory

# Run: Build and then execute the server
run: all
	@./$(BUILD_DIR)/$(BINARY)

# Clean: Wipe out the entire build directory
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Build directory deleted."

.PHONY: all run clean