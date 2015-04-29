#pragma once

#include "fourmath.h"
#include "component.h"

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

  ComponentBus _componentBus;

  Camera()
      : _movement(ORBIT) {
    _cameraMatrix.storeIdentity();
    _cameraPos.storeZero();

    bool success = _componentBus.RegisterOwnerData(
        std::string("orientation"), &_cameraMatrix, true);
    success &= _componentBus.RegisterOwnerData(std::string("position"), &_cameraPos, true);
    assert(success == true);
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
  void ApplyWorldRotation(float radians, Direction target, Direction source);

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

  void* operator new(size_t i)
  {
      return _aligned_malloc(i, 16);
  }

  void operator delete(void* p)
  {
      _aligned_free(p);
  }

  void Step(float fDeltaTime) {
    // after all this component stuff,
    // now I want to not do it this way and instead do crazy things
    // like make camera a component of render and chain stuff magically...
    _componentBus.Step(fDeltaTime);
  }

  ComponentBus& GetComponentBus() { return _componentBus; }
  static void TestComponents();

private:
  void RenormalizeCamera(Direction changeBasis);
  void PickBasisFromForward(const Direction changeBasis, Direction& firstOther,
      Direction& secondOther);
};

} // namespace fd
