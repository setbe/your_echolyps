# ==== texturepacks ====

T_DEFAULT = default

# ==== palettes ====

P_SOLARIAN = solarian24.png
P_QUAKE2 = quake-2.png

# ==== settings ====

TEXTUREPACK = $(T_DEFAULT)
PALETTE = 

TEXTURES_ARGS = $(TEXTUREPACK) $(PALETTE)

# ==== settings end ====

CXX = g++

DEBUG_CXXFLAGS = -std=c++20 -O0 -g
RELEASE_CXXFLAGS = \
  -std=c++20 -O3 -DNDEBUG \
  -flto \
  -march=native \
  -fno-ident

PUBLIC_CXXFLAGS = \
  -std=c++20  -O2 -DNDEBUG -DHI_PUBLIC \
  -flto \
  -mno-sse4 -mno-avx -mno-avx2 -mno-fma \
  -fno-ident

DEBUG_LDFLAGS =
RELEASE_LDFLAGS = -flto
PUBLIC_LDFLAGS = -flto

EXTERNAL_ROOT_DIR = ../../src/external
SRC_DIR = src
BUILD_DIR = ../../build
TARGET = $(BUILD_DIR)/textures

SRC_FILES := $(shell find $(SRC_DIR) $(EXTERNAL_ROOT_DIR) -type f \( -name "*.cpp" -o -name "*.c" \))
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

default: clean run

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
	./$(TARGET) $(TEXTURES_ARGS)
