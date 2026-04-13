CXX = clang++
CC  = clang

DEBUG_FLAGS = -std=c++17 -Wall -Weffc++ -Wextra -Werror -pedantic-errors -Wconversion -Wsign-conversion -ggdb -O0
RELEASE_FLAGS = -std=c++17 -Wall -Weffc++ -Wextra -Werror -pedantic-errors -Wconversion -Wsign-conversion -O3 -ffast-math -DNDEBUG
TARGET = main
BUILD_DIR = build

# Find all source files
include engine/engine.mk

CPP_SOURCES = $(ENGINE_SOURCES) \
							$(shell find \
							src \
							libs/audio_io/src \
							libs/device_io/src \
							deps/imgui \
							-name '*.cpp')
C_SOURCES = $(shell find deps/lua/src deps/linenoise -name '*.c')

# Object files (in build directory)
CPP_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
C_OBJECTS  = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ALL_OBJECTS = $(CPP_OBJECTS) $(MM_OBJECTS) $(C_OBJECTS)

# Add src/ to include search path
INCLUDES = $(ENGINE_INCLUDES) \
					 -Isrc \
					 -Ilibs/audio_io/include \
					 -Ilibs/audio_io/src \
					 -Ilibs/device_io/include \
					 -Ilibs/meh_utils/include \
					 -Ideps/lua/include \
					 -Ideps/linenoise \
					 -Ideps/imgui \
           -Ideps/imgui/backends \
           -Ideps/glfw/include

LDFLAGS = -framework AudioToolbox \
					-framework CoreAudio \
					-framework CoreFoundation \
					-framework CoreMIDI \
					-framework OpenGL \
					-framework Cocoa \
          -framework ApplicationServices \
          -framework IOKit \
					-Ldeps/glfw/lib -lglfw3

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

# Compile C sources
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -std=c11 $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(TARGET) $(BUILD_DIR)

.PHONY: debug release clean
