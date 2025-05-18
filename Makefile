.PHONY: all shaders font textures clean run release public mini

CXX = g++

SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/echolyps

SRC_FILES := $(shell find $(SRC_DIR) -type f \( -name "*.cpp" -o -name "*.c" \))
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Compilation flags
DEBUG_CXXFLAGS = -std=c++20 -O0 -g
RELEASE_CXXFLAGS = \
  -std=c++20 -O3 -march=native -DNDEBUG \
  -flto \
  -fno-exceptions -fno-use-cxa-atexit -fomit-frame-pointer \
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -fno-ident

PUBLIC_CXXFLAGS = \
  -std=c++20 -O2 -DNDEBUG -DHI_PUBLIC \
  -flto \
  -fno-exceptions -fno-use-cxa-atexit -fomit-frame-pointer \
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -fno-ident

MINI_CXXFLAGS = \
  -std=c++20 -Os -DNDEBUG \
  -flto \
  -fno-exceptions -fno-use-cxa-atexit -fomit-frame-pointer \
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -fno-ident

# Linker flags
DEBUG_LDFLAGS = -lX11 -lGL -ldl
RELEASE_LDFLAGS = -Wl,--gc-sections -Wl,-s -Wl,-z,norelro -Wl,-Bdynamic -lX11 -lGL -ldl
PUBLIC_LDFLAGS  = -Wl,--gc-sections -Wl,-s -Wl,-z,norelro -Wl,-Bdynamic -lX11 -lGL -ldl
MINI_LDFLAGS = -Wl,--gc-sections -Wl,-s -Wl,-z,norelro -Wl,-Bdynamic -lX11 -lGL -ldl

default: shaders clean run

shaders:
	$(MAKE) -C resources/shaders

font:
	$(MAKE) -C resources/fonts

textures:
	$(MAKE) -C resources/textures

all: CXXFLAGS = $(DEBUG_CXXFLAGS)
all: LDFLAGS = $(DEBUG_LDFLAGS)
all: $(BUILD_DIR) $(TARGET)

release: CXXFLAGS = $(RELEASE_CXXFLAGS)
release: LDFLAGS = $(RELEASE_LDFLAGS)
release: $(BUILD_DIR) $(TARGET)

public: CXXFLAGS = $(PUBLIC_CXXFLAGS)
public: LDFLAGS = $(PUBLIC_LDFLAGS)
public: $(BUILD_DIR) $(TARGET)

mini: CXXFLAGS = $(MINI_CXXFLAGS)
mini: LDFLAGS = $(MINI_LDFLAGS)
mini: $(BUILD_DIR) $(OBJ_FILES)
	mkdir -p $(dir $(TARGET))
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $(TARGET) $(LDFLAGS)
	strip --strip-all $(TARGET)
	objcopy --remove-section=.comment --remove-section=.note $(TARGET)
	upx --best --ultra-brute $(TARGET) -o $(TARGET)_mini
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
	./$(TARGET)
