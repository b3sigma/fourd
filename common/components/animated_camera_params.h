#pragma once

#include <algorithm>

#include "../component.h"

//void Usage {
//  Camera* pCamera; // Something that has a ComponentBus
//  pCamera->GetComponentBus().AddComponent(
//      new AnimatedCameraParams(-10.0f, 10.0f, 0.5f, 1.0f));
//}
namespace fd {

template <typename T>
class InterpValue {
public:
  T _targetVal;
  T _startVal;
  InterpValue(T start, T end) : _targetVal(end), _startVal(start) {}

  T GetCurrent(float interp) {
    // structured so that garbage _startVal still ends up at target
    return _targetVal - (T)((float)(_targetVal - _startVal) * (1.0f - interp));
  }
};

class AnimatedCameraParams : public Component {
protected:

  InterpValue<float> _wNear;
  InterpValue<float> _wFar;
  InterpValue<float> _wScreenSizeRatio;

  float _duration;

  float _current;

  float* _wNearOwner;
  float* _wFarOwner;
  float* _wScreenSizeRatioOwner;

public:
  AnimatedCameraParams(float wNear, float wFar, float wScreenSizeRatio,
      float duration)
      : _wNear(0.0f, wNear)
      , _wFar(0.0f, wFar)
      , _wScreenSizeRatio(0.0f, wScreenSizeRatio)
      , _duration(duration)
      , _current(0.0f)
  {
    assert(duration != 0.0f);
  }

  virtual ~AnimatedCameraParams() {
    assert(_duration != 0.0f); // just need a line for the debugger
  }

  virtual void OnConnected() {
    if(!m_ownerBus->GetOwnerData(std::string("wNear"), true, &_wNearOwner)
        || !m_ownerBus->GetOwnerData(std::string("wFar"), true, &_wFarOwner)
        || !m_ownerBus->GetOwnerData(std::string("wScreenSizeRatio"), true, &_wScreenSizeRatioOwner)) {
      assert(false);
      SelfDestruct();
    }
    _wNear._startVal = *_wNearOwner;
    _wFar._startVal = *_wFarOwner;
    _wScreenSizeRatio._startVal = *_wScreenSizeRatioOwner;

    RegisterSignal(std::string("Step"), this, &AnimatedCameraParams::OnStepSignal);
  }

  void OnStepSignal(float delta) {
    float timeDiff = fabs(fmod(_duration - _current, _duration + 0.00001f));
    float stepAmount = (::std::min)(timeDiff, delta);

    _current += stepAmount;

    float interp = _current / _duration;
    *_wNearOwner = _wNear.GetCurrent(interp);
    *_wFarOwner = _wFar.GetCurrent(interp);
    *_wScreenSizeRatioOwner = _wScreenSizeRatio.GetCurrent(interp);

    if(_current >= _duration && _duration > 0.0f) {
      // just make sure we end up at the right spot
      *_wNearOwner = _wNear.GetCurrent(1.0f);
      *_wFarOwner = _wFar.GetCurrent(1.0f);
      *_wScreenSizeRatioOwner = _wScreenSizeRatio.GetCurrent(1.0f);
      SelfDestruct();
    }
  }
};

} // namespace fd
