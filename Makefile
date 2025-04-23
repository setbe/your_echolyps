CXX = g++
DEBUG_CXXFLAGS   = -std=c++20 -O0 -g
RELEASE_CXXFLAGS = -std=c++20 -O3 -DNDEBUG -ffreestanding -nostdlib -fno-exceptions -fno-use-cxa-atexit -fomit-frame-pointer -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -march=native

DEBUG_LDFLAGS   = -lX11 -lGL -ldl
RELEASE_LDFLAGS = -flto -nostdlib -Wl,-e,_start -Wl,--gc-sections -Wl,-s -Wl,-z,norelro -lX11 -lGL

SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/echolyps

SRC_FILES := $(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

all: CXXFLAGS = $(DEBUG_CXXFLAGS)
all: LDFLAGS = $(DEBUG_LDFLAGS)
all: $(BUILD_DIR) $(TARGET)

release: CXXFLAGS = $(RELEASE_CXXFLAGS)
release: LDFLAGS = $(RELEASE_LDFLAGS)
release: $(BUILD_DIR) $(TARGET)

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
