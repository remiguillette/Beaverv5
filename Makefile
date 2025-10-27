# Compiler
CXX = g++
# Compiler flags
CXXFLAGS = -O2 -pipe -std=c++20 -Wall -Wextra -Wpedantic
# Include directories (add others if needed)
INCLUDES = -Iinclude `pkg-config --cflags gtk4`
# Linker flags (using pkg-config for GTK4 libraries)
LDFLAGS = `pkg-config --libs gtk4`
# Source files directory
SRC_DIR = src
# Object files directory
BUILD_DIR = build
# Executable name
TARGET = beaver_kiosk

# Find all .cpp files in the source directory
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
# Generate object file names based on source files
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Default target: build the executable
all: $(BUILD_DIR) $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Linking $(TARGET)..."

# Rule to compile .cpp files into .o object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo "Compiling $<..."

# Create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean target: remove build directory and executable
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Cleaned build files."

# Phony targets
.PHONY: all clean
