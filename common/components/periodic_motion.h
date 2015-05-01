#pragma once

#include <algorithm>

#include "../component.h"

//void Usage {
//  Camera* pCamera; // Something that has a ComponentBus
//  pCamera->GetComponentBus().AddComponent(
//      new PeriodicMotion((float)PI / 4.0f,
//          ::fd::Camera::INSIDE, ::fd::Camera::RIGHT, false));
//}
namespace fd {

class PeriodicMotion : public Component {
protected:
  Vec4f _direction;
  float _duration;
  float _phase;
  float _period;

  float _current; 
  Vec4f* _pOwnerPosition;

public:
  PeriodicMotion(float duration, float period, float phase, Vec4f direction)
      : _duration(duration)
      , _period(period)
      , _phase(phase)
      , _direction(direction)
      , _current(0.0f)
      , _pOwnerPosition(NULL)
  {
    assert(duration != 0.0f);
    assert(period != 0.0f);
  }

  virtual ~PeriodicMotion() {
    assert(_duration != 0.0f); // just need a line for the debugger
  }

  virtual void OnConnected() {
    if(!m_ownerBus->GetOwnerData(std::string("position"), true, &_pOwnerPosition)) {
      assert(false);
      SelfDestruct();
    }
    RegisterSignal(std::string("Step"), this, &PeriodicMotion::OnStepSignal);
  }

  // This is a little weird in that you want periodic motion, but you don't
  // want to set position directly. Thus, you are setting a velocity,
  // which modifies the position. 
  void OnStepSignal(float delta) {
    assert(_pOwnerPosition != NULL);

    float amount = (std::min)(delta, _duration - _current);

    float stepAmount = (::std::min)(_duration - _current, delta);
    float directionScalar = cos((_current / _period) + _phase) * amount;
    Vec4f posDelta = _direction * directionScalar; 
    (*_pOwnerPosition) += posDelta;

    _current += delta;
    if(_current >= _duration) {
      SelfDestruct();
    }
  }
};

} // namespace fd
