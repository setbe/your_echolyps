.PHONY: all shaders shaders-run clean-run clean run release public

CXX = g++

DEBUG_CXXFLAGS = -std=c++20 -O0 -g
RELEASE_CXXFLAGS = \
  -std=c++20 -O3 -DNDEBUG \
  -ffreestanding -fomit-frame-pointer \
  -fno-exceptions -fno-use-cxa-atexit \
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -march=native \
  -fno-ident

PUBLIC_CXXFLAGS = \
  -std=c++20 -O2 -DNDEBUG -DHI_PUBLIC \
  -ffreestanding -fomit-frame-pointer \
  -fno-exceptions -fno-use-cxa-atexit \
  -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -mno-sse4 -mno-avx -mno-avx2 -mno-fma \
  -fno-ident

DEBUG_LDFLAGS = -lX11 -lGL -ldl
RELEASE_LDFLAGS = -Wl,--gc-sections -Wl,-s -Wl,-z,norelro -Wl,-Bdynamic -lX11 -lGL -ldl
PUBLIC_LDFLAGS  = -Wl,--gc-sections -Wl,-s -Wl,-z,norelro -Wl,-Bdynamic -lX11 -lGL -ldl

SRC_DIR = src
BUILD_DIR = ../../build
TARGET = $(BUILD_DIR)/fonts

SRC_FILES := $(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

default: shaders shaders-run clean-run run

shaders:
	cd resources/shaders/ && make clean

shaders-run:
	cd resources/shaders/ && make run

clean-run:
	$(MAKE) clean

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
	./$(TARGET)
