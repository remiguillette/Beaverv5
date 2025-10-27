CXX := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -I./include

GTK_CFLAGS := $(shell pkg-config --cflags gtk4 2>/dev/null)
GTK_LIBS := $(shell pkg-config --libs gtk4 2>/dev/null)

WEBKIT_CFLAGS := $(shell pkg-config --cflags webkit2gtk-4.1 2>/dev/null)
WEBKIT_LIBS := $(shell pkg-config --libs webkit2gtk-4.1 2>/dev/null)

ifeq ($(WEBKIT_CFLAGS),)
WEBKIT_CFLAGS := $(shell pkg-config --cflags webkit2gtk-4.0 2>/dev/null)
WEBKIT_LIBS := $(shell pkg-config --libs webkit2gtk-4.0 2>/dev/null)
endif

CXXFLAGS += $(GTK_CFLAGS) $(WEBKIT_CFLAGS)
LDFLAGS += $(GTK_LIBS) $(WEBKIT_LIBS)

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
