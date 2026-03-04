#include "dsp/Wavetable.h"
#include "dsp/Math.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace dsp::wavetable {

// ================================
// Lifecycle
// ================================
WavetableBank* createWavetableBank(uint32_t frameCount, const char* name) {
  if (frameCount == 0 || frameCount > MAX_FRAMES) {
    printf("createWavetableBank: invalid frameCount %u\n", frameCount);
    return nullptr;
  }
  WavetableBank* bank = new WavetableBank();
  bank->frames = new WavetableFrame[frameCount];
  bank->frameCount = frameCount;

  std::strncpy(bank->name, name, MAX_BANK_NAME_LEN - 1);
  bank->name[MAX_BANK_NAME_LEN - 1] = '\0';

  return bank;
}

bool destroyWavetableBank(WavetableBank* bank) {
  if (!bank) {
    printf("destroyWavetableBank: null bank\n");
    return false;
  }
  delete[] bank->frames;
  delete bank;
  return true;
}

/* ================================
 * Mip selection
 * ================================
 * Returns a continuous float mip level. The integer part selects mip table A;
 * integer+1 selects mip table B. The fractional part is the blend weight.
 *
 * Relationship: one octave up = phase increment doubles = mip level +1.
 * So mip = log2(phaseIncrement) gives a continuous float that tracks pitch.
 * fastLog2 gives this in O(1) with ~0.01 error — well within audible tolerance.
 *
 * MUST be in TABLE_SIZE units (table positions/sample)
 */
float selectMipLevel(float phaseIncrement) {
  if (phaseIncrement <= 1.0f)
    return 0.0f;

  float mip = math::fastLog2(phaseIncrement);

  // Clamped to MAX_MIP_LEVELS-2 so mipB = floor(mip)+1 never goes out of bounds
  return std::clamp(mip, 0.0f, float(MAX_MIP_LEVELS - 2));
}

/* ===================================
 * Table lookup — linear interpolation
 * ===================================
 * _phase_ is in table-space [0, TABLE_SIZE_F). The caller (readWavetable)
 * converts normalized [0, 1.0) phase to table-space by multiplying by
 * TABLE_SIZE_F before calling here. This keeps the multiply out of any inner
 * loop that calls readTable multiple times (e.g. dual-mip reads 2–4 times per
 * sample).
 *
 * TABLE_MASK wraps iB at 2047 → 0. iA is already in [0, TABLE_SIZE-1] by
 * construction (truncation of a value in [0, TABLE_SIZE_F)). Bitmask is valid
 * because TABLE_SIZE is a power of 2.*/

float readTable(const float* table, float phase) {
  uint32_t iA = static_cast<uint32_t>(phase);
  uint32_t iB = (iA + 1) & TABLE_MASK;         // wrap 2047 → 0
  float frac = phase - static_cast<float>(iA); // fractional part [0, 1)

  return table[iA] + frac * (table[iB] - table[iA]);
}

/* ================================
 * Table read — dual-mip linear interpolation
 * ================================
 *
 * Two reads per mip level (linear interp within table) × two mip levels = 4
 * reads/sample. Same cost as cubic at a single mip, but correct under pitch
 * modulation and free of mip transition artifacts. See Design Decisions for the
 * full trade-off explanation.
 *
 * FM phase offset: float in cycles. Added to the
 * normalized phase and wrapped via floorf — handles any magnitude and sign in a
 * single instruction.
 *
 * Frame interpolation: scanF maps [0,1] onto [0, frameCount-1]. Linear blend
 * between frameA and frameB is sufficient — morphing is perceptually smooth at
 * block-rate.
 */
float readWavetable(const WavetableBank* bank,
                    float phase,
                    float mipF,
                    float effectiveScanPos,
                    float fmPhaseOffset) {
  // Apply FM offset and wrap to [0, 1.0).
  // floorf handles any magnitude and sign — the reason phases are normalized.
  // Convert to table-space once here; all readTable calls below use tablePhase.
  float rawPhase = phase + fmPhaseOffset;
  float wrappedPhase = rawPhase - floorf(rawPhase);

  float tablePhase = wrappedPhase * TABLE_SIZE_F; // [0, TABLE_SIZE)

  // Mip blend
  int mA = int(mipF);
  int mB = mA + 1;
  float mFrac = mipF - float(mA);

  // Single-frame fast path
  if (bank->frameCount == 1) {
    float sA = readTable(bank->frames[0].mips[mA], tablePhase);
    float sB = readTable(bank->frames[0].mips[mB], tablePhase);
    return sA + mFrac * (sB - sA);
  }

  // Multi-frame: interpolate between adjacent frames
  float scanF = effectiveScanPos * float(bank->frameCount - 1);

  int frameA = std::min(static_cast<int>(scanF), static_cast<int>(bank->frameCount) - 2);
  int frameB = frameA + 1;
  float fFrac = scanF - float(frameA);

  // 4 table reads — (2 frames) × (2 mip levels), then blend along both axes
  float fAmA = readTable(bank->frames[frameA].mips[mA], tablePhase);
  float fBmA = readTable(bank->frames[frameB].mips[mA], tablePhase);
  float fAmB = readTable(bank->frames[frameA].mips[mB], tablePhase);
  float fBmB = readTable(bank->frames[frameB].mips[mB], tablePhase);

  float sA = fAmA + fFrac * (fBmA - fAmA); // frame lerp at mip A
  float sB = fAmB + fFrac * (fBmB - fAmB); // frame lerp at mip B
  return sA + mFrac * (sB - sA);           // mip lerp
}

float processWavetableOsc(const WavetableBank* bank,
                          float& phase,
                          float mipF,
                          float effectiveScanPos,
                          float fmPhaseOffset,
                          float pitchIncrement) {
  float sample = readWavetable(bank, phase, mipF, effectiveScanPos, fmPhaseOffset);
  phase += pitchIncrement;
  phase -= floorf(phase);

  return sample;
}

} // namespace dsp::wavetable
