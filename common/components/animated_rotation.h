#pragma once

#include <algorithm>

#include "../component.h"

//void Usage {
//  Camera* pCamera; // Something that has a ComponentBus
//  pCamera->GetComponentBus().AddComponent(
//      new AnimatedRotation((float)PI / 4.0f,
//          ::fd::Camera::INSIDE, ::fd::Camera::RIGHT, false));
//}
namespace fd {

class AnimatedRotation : public Component {
protected:
  float _radians;
  int _targetDirection;
  int _sourceDirection;
  float _duration;
  bool  _isWorldRotation;

  float _current;
  Mat4f* _pOwnerMatrix;


public:
  AnimatedRotation(float radians, int targetDir, int sourceDir, 
      float duration, bool worldRotation)
      : _radians(radians)
      , _targetDirection(targetDir)
      , _sourceDirection(sourceDir)
      , _duration(duration)
      , _isWorldRotation(worldRotation)
      , _current(0.0f)
      , _pOwnerMatrix(NULL)
  {
    assert(duration != 0.0f);
  }

  virtual ~AnimatedRotation() {
    assert(_duration != 0.0f); // just need a line for the debugger
  }

  virtual void OnConnected() {
    if(!_bus->GetOwnerData(std::string("orientation"), true, &_pOwnerMatrix)) {
      assert(false);
      SelfDestruct();
    }
    RegisterSignal(std::string("Step"), this, &AnimatedRotation::OnStepSignal);
  }

  void OnStepSignal(float delta) {
    assert(_pOwnerMatrix != NULL);

    float stepAmount = (::std::min)(_duration - _current, delta);

    Mat4f rot;
    float amount = _radians / _duration * stepAmount;
    rot.buildRotation(amount, _targetDirection, _sourceDirection);

    if (_isWorldRotation) {
      *_pOwnerMatrix = (*_pOwnerMatrix) * rot.transpose();
    } else {
      *_pOwnerMatrix = rot.transpose() * (*_pOwnerMatrix);
    }

    _current += delta;
    if(_current >= _duration) {
      SelfDestruct();
    }
  }

private:
  AnimatedRotation() {}
  // don't allow copies
  AnimatedRotation(const AnimatedRotation& copy) { assert(false); }

};

} // namespace fd
