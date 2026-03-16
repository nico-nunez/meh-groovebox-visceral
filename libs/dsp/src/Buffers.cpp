#include "dsp/Buffers.h"

#include <cstddef>
#include <cstdint>

namespace dsp::buffers {
void initStereoBuffer(StereoBuffer& buffer, size_t size) {
  buffer.buffer = new float[size * 2]();
  buffer.left = buffer.buffer;
  buffer.right = buffer.left + size;
}

StereoBuffer createStereoBufferSlice(const StereoBuffer& buffer, uint32_t offset) {
  return {nullptr, buffer.left + offset, buffer.right + offset};
}

void destroyStereoBuffer(StereoBuffer& buffer) {
  delete[] buffer.buffer;
  buffer.buffer = nullptr;
  buffer.left = nullptr;
  buffer.right = nullptr;
}

} // namespace dsp::buffers
