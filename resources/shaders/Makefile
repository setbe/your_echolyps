CXX = g++
RELEASE_CXXFLAGS = -std=c++20 -DNDEBUG -O2 # -march=native
DEBUG_CXXFLAGS = -std=c++20 -O0 -g
RELEASE_LDFLAGS = -flto
DEBUG_LDFLAGS =
SRC_DIR = src
BUILD_DIR = ../../build
TARGET = $(BUILD_DIR)/shaders

SRC_FILES := $(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Fast build
all: CXXFLAGS = $(DEBUG_CXXFLAGS)
all: LDFLAGS = $(DEBUG_LDFLAGS)
all: $(BUILD_DIR) $(TARGET)

# Release build
release: CXXFLAGS = $(RELEASE_CXXFLAGS)
release: LDFLAGS = $(RELEASE_LDFLAGS)
release: $(BUILD_DIR) $(TARGET)
	strip --strip-all $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Compile every `.cpp` into `.o`
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(TARGET)
