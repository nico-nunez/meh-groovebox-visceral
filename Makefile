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
							libs/file_watch/src \
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
           -Ilibs/file_watch/include \
           -Ilibs/file_watch/src \
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
          -framework CoreServices \
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

# ====================================
# Test target
# ====================================
TEST_TARGET  = test_runner
TEST_BUILD   = $(BUILD_DIR)/test

TEST_CPP_SOURCES = $(ENGINE_SOURCES) \
                   $(shell find \
                   libs/audio_io/src \
                   libs/device_io/src \
									 libs/file_watch/src \
                   deps/imgui \
                   -name '*.cpp') \
                   $(filter-out src/main.cpp, $(shell find src -name '*.cpp')) \
                   $(shell find tests -name '*.cpp')

TEST_CPP_OBJECTS = $(patsubst %.cpp,$(TEST_BUILD)/%.o,$(TEST_CPP_SOURCES))
TEST_C_OBJECTS   = $(patsubst %.c,$(TEST_BUILD)/%.o,$(C_SOURCES))

test: CXXFLAGS = $(DEBUG_FLAGS)
test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_CPP_OBJECTS) $(TEST_C_OBJECTS)
	$(CXX) $(LDFLAGS) -o $(TEST_TARGET) $(TEST_CPP_OBJECTS) $(TEST_C_OBJECTS)

$(TEST_BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Itests -c $< -o $@

$(TEST_BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -std=c11 $(INCLUDES) -c $< -o $@

.PHONY: debug release clean test

# ====================================
# LuaLS Stub Generation
# ====================================
LUALS_STUB_GENERATOR = $(BUILD_DIR)/generate_luals_stubs

LUALS_STUB_GENERATOR_SOURCES = \
	src/app/AppParams.cpp \
	src/app/doc/DocMetadata.cpp \
	src/lua/metadata/LuaRuntimeMetadata.cpp \
	tools/luals/generate_luals_stubs.cpp 

$(LUALS_STUB_GENERATOR): $(LUALS_STUB_GENERATOR_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(DEBUG_FLAGS) $(INCLUDES) -o $@ $(LUALS_STUB_GENERATOR_SOURCES)

luals-stubs: $(LUALS_STUB_GENERATOR)
	$(LUALS_STUB_GENERATOR) --out generated/luals

check-luals-stubs: $(LUALS_STUB_GENERATOR)
	$(LUALS_STUB_GENERATOR) --out generated/luals --check

.PHONY: debug release clean test luals-stubs check-luals-stubs
