#include "app/Mixer.h"

namespace app::mixer {

MixerSnapshotSwapResult prepareMixerSnapshotSwap(MixerState& mixer,
                                                 const MixerSnapshot& targetSnapshot) {
  bool expected = false;
  if (!mixer.writeInFlight.compare_exchange_strong(expected,
                                                   true,
                                                   std::memory_order_acq_rel,
                                                   std::memory_order_acquire)) {
    return {false, "mixer write already in flight"};
  }

  if (mixer.pendingReady.load(std::memory_order_acquire)) {
    mixer.writeInFlight.store(false, std::memory_order_release);
    return {false, "mixer snapshot pending"};
  }

  mixer.pending = targetSnapshot;
  return {true, nullptr};
}

MixerSnapshotSwapResult commitMixerSnapshotSwap(MixerState& mixer) {
  if (!mixer.writeInFlight.load(std::memory_order_acquire))
    return {false, "no mixer write in flight"};

  mixer.pendingReady.store(true, std::memory_order_release);
  mixer.writeInFlight.store(false, std::memory_order_release);
  return {true, nullptr};
}

void abortMixerSnapshotSwap(MixerState& mixer) {
  mixer.writeInFlight.store(false, std::memory_order_release);
}

void publishPendingMixerSnapshotIfReady(MixerState& mixer) {
  if (!mixer.pendingReady.load(std::memory_order_acquire))
    return;

  mixer.current = mixer.pending;
  mixer.pendingReady.store(false, std::memory_order_release);
}

} // namespace app::mixer
