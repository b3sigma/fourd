#pragma once

#include "fourmath.h"

namespace fd {

class Camera {
public:
  Mat4f _cameraMatrix;
  Vec4f _cameraPos;

  enum Direction {
    RIGHT = 0, UP = 1, FORWARD = 2, INSIDE = 3,
  };

  enum MovementMode {
    LOOK = 1, ORBIT = 2,
  };

  MovementMode _movement;
  Vec4f _cameraLookAt;

  Camera()
      : _movement(ORBIT) {
    _cameraMatrix.storeIdentity();
    _cameraPos.storeZero();
  }

  Camera(MovementMode mode)
      : _movement(mode) {
    _cameraMatrix.storeIdentity();
    _cameraPos.storeZero();
  }

  void SetCameraPosition(Vec4f position) {
    _cameraPos = position;
  }

  void ApplyOrbitInput(float radians, Direction direction);
  void ApplyRotationInput(float radians, Direction target, Direction source);
  void ApplyRollInput(float radians, Direction target, Direction source);

  void ApplyTranslationInput(float amount, Direction direction);

  void printIt();

  const Mat4f& getCameraMatrix() const {
    return _cameraMatrix;
  }
  const Vec4f& getCameraPos() const {
    return _cameraPos;
  }

  MovementMode getMovementMode() const { return _movement; }
  void setMovementMode(MovementMode mode) {
    _movement = mode;
  }

private:
  void RenormalizeCamera(Direction changeBasis);
  void PickBasisFromForward(const Direction changeBasis, Direction& firstOther,
      Direction& secondOther);
};

} // namespace fd
