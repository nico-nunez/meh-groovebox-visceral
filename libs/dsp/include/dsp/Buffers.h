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

} // namespace dsp::buffers
