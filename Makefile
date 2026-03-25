CXX = clang++
CC  = clang

DEBUG_FLAGS = -std=c++17 -Wall -Weffc++ -Wextra -Werror -pedantic-errors -Wconversion -Wsign-conversion -ggdb -O0
RELEASE_FLAGS = -std=c++17 -Wall -Weffc++ -Wextra -Werror -pedantic-errors -Wconversion -Wsign-conversion -O3 -ffast-math -DNDEBUG
TARGET = main
BUILD_DIR = build

# Find all source files
CPP_SOURCES = $(shell find src libs/audio_io/src libs/device_io/src libs/synth_io/src libs/dsp/src libs/json/src -name '*.cpp')
MM_SOURCES = $(shell find libs/device_io/src -name '*.mm')
C_SOURCES = $(shell find libs/lua/src libs/linenoise -name '*.c')

# Object files (in build directory)
CPP_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
MM_OBJECTS = $(patsubst %.mm,$(BUILD_DIR)/%.o,$(MM_SOURCES))
C_OBJECTS  = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ALL_OBJECTS = $(CPP_OBJECTS) $(MM_OBJECTS) $(C_OBJECTS)

# Add src/ to include search path
INCLUDES = -Isrc -Ilibs/audio_io/include -Ilibs/audio_io/src \
					 -Ilibs/device_io/include \
					 -Ilibs/synth_io/include \
					 -Ilibs/dsp/include \
					 -Ilibs/json/include \
					 -Ilibs/lua/include \
					 -Ilibs/linenoise

LDFLAGS = -framework CoreAudio \
					-framework AudioToolbox \
					-framework ApplicationServices \
					-framework Cocoa \
					-framework CoreMIDI \
					-framework CoreFoundation

# Objective-C++ flags (subset of warnings, some don't apply well to ObjC++)
OBJCXX_FLAGS = -std=c++17 -fobjc-arc -Wall -Wextra -Werror

OLD ?= 0
debug: CXXFLAGS = $(DEBUG_FLAGS) -DOLD=$(OLD)
debug: $(TARGET)

release: CXXFLAGS = $(RELEASE_FLAGS)
release: $(TARGET)

# Link all objects
$(TARGET): $(ALL_OBJECTS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(ALL_OBJECTS)

# Compile C++ sources
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile Objective-C++ sources
$(BUILD_DIR)/%.o: %.mm
	@mkdir -p $(dir $@)
	$(CXX) -xobjective-c++ $(OBJCXX_FLAGS) $(INCLUDES) -c $< -o $@

# Compile C sources
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -std=c11 $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(TARGET) $(BUILD_DIR)

.PHONY: debug release clean
