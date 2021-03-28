#include <mods/ai/PolyBLEP.h>
#include <math.h>
#include <hal/simd.h>
#include <od/config.h>

namespace ai
{

  PolyBLEPOscillator::PolyBLEPOscillator()
  {
    addInput(mVoltPerOctave);
    addInput(mSync);
    addOutput(mOutput);
    addInput(mFundamental);
    addParameter(mPhase);
    mPhase.enableSerialization();
  }

  PolyBLEPOscillator::~PolyBLEPOscillator()
  {
  }

  // [Aliasing Saw]: 0.7360% (19644 ticks, 374 Hz)
  void PolyBLEPOscillator::process()
  {
    // Get pointers to the input and output buffers.
    float *octave = mVoltPerOctave.buffer();
    float *out = mOutput.buffer();
    float *sync = mSync.buffer();
    float *freq = mFundamental.buffer();
    // Some constants in SIMD vector form.
    float32x4_t glog2 = vdupq_n_f32(FULLSCALE_IN_VOLTS * logf(2.0f));
    float32x4_t two = vdupq_n_f32(2.0f);
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t zero = vdupq_n_f32(0.0f);
    float32x4_t negOne = vdupq_n_f32(-1.0f);
    float32x4_t sr = vdupq_n_f32(globalConfig.samplePeriod);
    // Restore phase from previous call.
    float phase = mPhase.value();



    // Step through buffers in multiples of 4 (SIMD vector size).
    for (int i = 0; i < FRAMELENGTH; i += 4)
    {
      //// 1. Calculate phase increment

      // Load the V/oct input into a SIMD vector.
      float32x4_t q = vld1q_f32(octave + i);
      // Load the linear freq (f0) input into a SIMD vector.
      float32x4_t dP = sr * vld1q_f32(freq + i);
      // Clamp the V/oct input to [-1,1] (i.e. -10V to 10V)
      q = vminq_f32(one, q);
      q = vmaxq_f32(zero, q); // ai: clamp to zero
      // Scale the linear frequency by the V/oct input
      q = dP * simd_exp(q * glog2);

      // Accumulate phase while handling sync
      float tmp[4];
      uint32_t syncTrue[4];
      // Load the sync signal into a SIMD vector.
      float32x4_t s = vld1q_f32(sync + i);
      // Compare sync vector to zero. 
      vst1q_u32(syncTrue, vcgtq_f32(s, zero)); // syncTrue = s > 0
      // Load phase increment vector into regular array for element-wise access.
      vst1q_f32(tmp, q);
      for (int j = 0; j < 4; j++)
      {
        // Accumulate the phase increments.
        float this_samp = next_samp;
        next_samp = 0; 
        phase += tmp[j];
        if (syncTrue[j])
        {
          // Sync signal detected.  Reset phase to zero.
          phase = 0;
        }

        else if (phase >= 1.0f){
          phase -= 1.0f;
          float t = phase / tmp[j];
          this_samp -= ThisBlepSample(t);
          next_samp -= NextBlepSample(t);
        }

        next_samp += phase;
        this_samp = (this_samp * 2.0f) - 1.0f;
        lp_state += 0.3f * (this_samp - lp_state);
        tmp[j] = lp_state; // ai: accumulate phase (because it has already been wrapped)!
      }
      // Store accumulated phase back into SIMD vector.
      q = vld1q_f32(tmp);

      //// 2. Wrap to [0,1] without branching.
      // // q is in [-inf, inf]
      // q = vsubq_f32(q, vcvtq_f32_s32(vcvtq_s32_f32(q))); // q = q - (int)q;
      // // q is in [-1, 1]
      // q += one;
      // // q is in [0, 2]
      // q = vsubq_f32(q, vcvtq_f32_s32(vcvtq_s32_f32(q))); // q = q - (int)q;
      // // q is finally in [0,1]

      //// 3. Calculate unbiased ramp.
      // q = vsubq_f32(two * q, one); // q = 2*q - 1

      // Store the result in the out buffer.
      vst1q_f32(out + i, q);
    }

    // Save the phase for the next call.
    phase -= (int)phase;
    mPhase.hardSet(phase);
  }

} /* namespace od */
