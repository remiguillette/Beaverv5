CXX := $(shell command -v clang++ || command -v g++)
ifeq ($(CXX),)
$(error No suitable C++ compiler found (clang++ or g++))
endif
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -I./include

WEBKIT_PKG := $(shell pkg-config --exists webkit2gtk-4.1 && echo webkit2gtk-4.1)
ifeq ($(WEBKIT_PKG),)
WEBKIT_PKG := $(shell pkg-config --exists webkit2gtk-4.0 && echo webkit2gtk-4.0)
endif

ifeq ($(WEBKIT_PKG),)
$(warning WebKitGTK development package not found. Build will likely fail when compiling UI sources.)
else
CXXFLAGS += $(shell pkg-config --cflags $(WEBKIT_PKG))
LDFLAGS += $(shell pkg-config --libs $(WEBKIT_PKG))
endif

TARGET := beaver_kiosk
SRC_DIR := src
OBJ_DIR := build

SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Clean complete!"

run: $(TARGET)
	./$(TARGET)
