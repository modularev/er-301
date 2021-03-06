#include <core/objects/oscillators/SawtoothOscillator.h>
#include <math.h>
#include <hal/simd.h>
#include <od/config.h>

namespace od
{

  SawtoothOscillator::SawtoothOscillator()
  {
    addInput(mVoltPerOctave);
    addInput(mSync);
    addOutput(mOutput);
    addInput(mFundamental);
    addParameter(mPhase);
    mPhase.enableSerialization();
  }

  SawtoothOscillator::~SawtoothOscillator()
  {
  }

  // [Aliasing Saw]: 0.7360% (19644 ticks, 374 Hz)
  void SawtoothOscillator::process()
  {
    float *octave = mVoltPerOctave.buffer();
    float *out = mOutput.buffer();
    float *sync = mSync.buffer();
    float *freq = mFundamental.buffer();
    float32x4_t glog2 = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
    float32x4_t two = vdupq_n_f32(2.0f);
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t zero = vdupq_n_f32(0.0f);
    float32x4_t negOne = vdupq_n_f32(-1.0f);
    float32x4_t sr = vdupq_n_f32(globalConfig.samplePeriod);
    // Restore phase from previous call.
    float phase = mPhase.value();

    for (int i = 0; i < FRAMELENGTH; i += 4)
    {
      //// Calculate phase increment

      // Load the V/oct input
      float32x4_t q = vld1q_f32(octave + i);
      // Load the freq input
      float32x4_t dP = sr * vld1q_f32(freq + i);
      // Clamp the V/oct input to [-1,1] (i.e. -10V to 10V)
      q = vminq_f32(one, q);
      q = vmaxq_f32(negOne, q);
      // Convert to linear phase increment
      q = dP * simd_exp(q * glog2);

      // Accumulate phase while handling sync
      float tmp[4];
      uint32_t syncTrue[4];
      float32x4_t s = vld1q_f32(sync + i);
      vst1q_u32(syncTrue, vcgtq_f32(s, zero));
      // Load SIMD vector into regular array for element-wise access.
      vst1q_f32(tmp, q);
      for (int j = 0; j < 4; j++)
      {
        phase += tmp[j];
        if (syncTrue[j])
        {
          // Sync signal detected.  Reset phase to zero.
          phase = 0;
        }
        tmp[j] = phase;
      }
      // Store accumulated phase back into SIMD vector.
      q = vld1q_f32(tmp);

      //// Wrap to [0,1] without branching.

      // q is in [-inf, inf]
      q = vsubq_f32(q, vcvtq_f32_s32(vcvtq_s32_f32(q)));
      // q is in [-1, 1]
      q += one;
      // q is in [0, 2]
      q = vsubq_f32(q, vcvtq_f32_s32(vcvtq_s32_f32(q)));
      // q is finally in [0,1]

      // Calculate unbiased ramp.
      q = vsubq_f32(two * q, one);

      // Store the result in the out buffer.
      vst1q_f32(out + i, q);
    }

    phase -= (int)phase;
    mPhase.hardSet(phase);
  }

} /* namespace od */
