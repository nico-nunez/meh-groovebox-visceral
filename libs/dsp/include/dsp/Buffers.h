#pragma once

#include <cstddef>
#include <cstdint>

namespace dsp::buffers {

struct StereoBuffer {
  float* buffer = nullptr;
  float* left = nullptr;
  float* right = nullptr;
};

void initStereoBuffer(StereoBuffer& buffer, size_t size);
StereoBuffer createStereoBufferSlice(const StereoBuffer& buffer, uint32_t offset);
void destroyStereoBuffer(StereoBuffer& buffer);

// Circular Stereo Buffer where size is always power of 2
struct StereoRingBuffer : StereoBuffer {
  size_t size = 0;
  size_t mask = 0;
};

void initStereoRingBuffer(StereoRingBuffer& cb, size_t requestedSize);
void destroyStereoRingBuffer(StereoRingBuffer& cb);

} // namespace dsp::buffers
