#pragma once

#include <od/objects/Object.h>

namespace ai
{

inline float ThisBlepSample(float t) {
  return 0.5f * t * t;
}

inline float NextBlepSample(float t) {
  t = 1.0f - t;
  return -0.5f * t * t;
}

  class PolyBLEPOscillator : public od::Object
  {
  public:
    PolyBLEPOscillator();
    virtual ~PolyBLEPOscillator();
#ifndef SWIGLUA
    virtual void process();
    od::Inlet mVoltPerOctave{"V/Oct"};
    od::Inlet mSync{"Sync"};
    od::Outlet mOutput{"Out"};
    od::Inlet mFundamental{"Fundamental"};
    od::Parameter mPhase{"Phase", 0.0f};
#endif
  protected:
    float next_samp = 0;
    float lp_state = 0;
  };

} /* namespace ai */
