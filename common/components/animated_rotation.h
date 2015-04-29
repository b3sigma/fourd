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
  Mat4f* _ownerMatrix;

public:
  AnimatedRotation(float radians, int targetDir, int sourceDir, 
      float duration, bool worldRotation)
      : Component() 
      , _radians(radians)
      , _targetDirection(targetDir)
      , _sourceDirection(sourceDir)
      , _duration(duration)
      , _isWorldRotation(worldRotation)
      , _current(0.0f)
      , _ownerMatrix(NULL)
  {}

  virtual void OnConnected() {
    if(!_bus->GetOwnerData<Mat4f>("orientation", true, &_ownerMatrix)) {
      SelfDestruct();
    }
  }

  void OnStepSignal(float delta) {
    assert(_ownerMatrix != NULL);

    float stepAmount = std::min(_duration - _current, delta);

    Mat4f rot;
    float amount = _radians / _duration * stepAmount;
    rot.buildRotation(amount, _targetDirection, _sourceDirection);

    if (_isWorldRotation) {
      *_ownerMatrix = (*_ownerMatrix) * rot.transpose();
    } else {
      *_ownerMatrix = rot.transpose() * (*_ownerMatrix);
    }

    _current += delta;
    if(_current > _duration) {
      SelfDestruct();
    }
  }
};

} // namespace fd
