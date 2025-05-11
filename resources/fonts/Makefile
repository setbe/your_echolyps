# ==== fonts ====

FONT_UNSCII = unscii/unscii-16-full.ttf

# ==== fonts ====
# ==== settings ====

FONT_ARGS = --size=16 --$(FONT_UNSCII)-en --$(FONT_UNSCII)-ua #--$(FONT_UNSCII)-jp

# ==== settings ====

.PHONY: all clean run release public

CXX = g++

DEBUG_CXXFLAGS = -std=c++20 -O0 -g
RELEASE_CXXFLAGS = \
  -std=c++20 -O3 -DNDEBUG \
  -march=native \
  -fno-ident

PUBLIC_CXXFLAGS = \
  -std=c++20 -O2 -DNDEBUG -DHI_PUBLIC \
  -mno-sse4 -mno-avx -mno-avx2 -mno-fma \
  -fno-ident

DEBUG_LDFLAGS =
RELEASE_LDFLAGS = -Wl,--gc-sections -Wl,-Bdynamic
PUBLIC_LDFLAGS  = -Wl,--gc-sections -Wl,-Bdynamic

EXTERNAL_ROOT_DIR = ../../src/external
SRC_DIR = src
BUILD_DIR = ../../build
TARGET = $(BUILD_DIR)/fonts

SRC_FILES := $(shell find $(SRC_DIR) $(EXTERNAL_ROOT_DIR) -type f \( -name "*.cpp" -o -name "*.c" \))
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

default: clean run

all: CXXFLAGS = $(DEBUG_CXXFLAGS)
all: LDFLAGS = $(DEBUG_LDFLAGS)
all: $(BUILD_DIR) $(TARGET)

release: CXXFLAGS = $(RELEASE_CXXFLAGS)
release: LDFLAGS = $(RELEASE_LDFLAGS)
release: $(BUILD_DIR) $(TARGET)

public: CXXFLAGS = $(PUBLIC_CXXFLAGS)
public: LDFLAGS = $(PUBLIC_LDFLAGS)
public: $(BUILD_DIR) $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(TARGET) $(FONT_ARGS)
