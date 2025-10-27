CXX ?= g++
PKG_CONFIG ?= pkg-config
CXXFLAGS ?= -O2 -pipe
CXXFLAGS += -std=c++20 -Wall -Wextra -Wpedantic $(shell $(PKG_CONFIG) --cflags gtk4)
LDFLAGS += $(shell $(PKG_CONFIG) --libs gtk4)

TARGET = beaver_kiosk
SRC_DIR = src
OBJ_DIR = build
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Clean complete!"

run: $(TARGET)
	./$(TARGET)
