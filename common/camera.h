#pragma once

#include "fourmath.h"
#include "component.h"

namespace fd {

class Camera {
public:
  Mat4f _cameraMatrix;
  Vec4f _cameraPos;

  Mat4f _renderMatrix; // camera matrix plus any extra like eye shift
  Vec4f _renderPos;    // camera pos plus any extra like eye offset

  Vec2i _screenBounds;
  Mat4f _zProjectionMatrix;
  Mat4f _fourToThree; // yeah went individual code instead of this so far

  float _zNear;
  float _zFar;
  float _zFov;

  float _wNear;
  float _wFar;
  // a different method than fov, near size / far size
  float _wScreenSizeRatio;  
  bool _wProjectionEnabled;
 
  enum Direction {
    RIGHT = 0, UP = 1, FORWARD = 2, INSIDE = 3,
  };
  
  enum MovementMode {
    LOOK = 1, ORBIT = 2, WALK = 3,
  };

  MovementMode _movement;

  Vec4f _cameraLookAt; // only used in orbit mode

  float _yaw; // only used in walk mode
  float _pitch; // only used in walk mode
  Mat4f _yawPitchTrans; // only used in walk mode
  Mat4f _yawTrans; // only used in vr walk mode
  Vec4f _pushVelocity; // walk mode
  bool _collidingLastFrame;
  Vec4f _velocity;

  bool _nextSimpleSlicePositive;

  ComponentBus _componentBus;

  Camera* _startingCameraCopy;

  Camera();
  ~Camera();

  //Camera(MovementMode mode)
  //    : _movement(mode) {
  //  _cameraMatrix.storeIdentity();
  //  _cameraPos.storeZero();
  //}

  void UpdateRenderMatrix(Mat4f* lookOffset, Vec4f* posOffset);
  void SetZProjection(int width, int height, 
      float zFov, float zNear, float zFar);
  void SetWProjection(float wNear, float wFar, float wScreenSizeRatio,
      float animateTime = 0.0f);

  void SetCameraPosition(Vec4f position) {
    _cameraPos = position;
  }

  void MarkStartingPosition();
  void RestartGameState();

  void ApplyOrbitInput(float radians, Direction direction);
  void ApplyRotationInput(float radians, Direction target, Direction source);
  void ApplyRollInput(float radians, Direction target, Direction source);
  void ApplyWorldRotation(float radians, Direction target, Direction source);
  void ApplyYawInput(float radians);
  void ApplyPitchInput(float radians);
  void RebuildOrientationFromYawPitch();

  void ApplyTranslationInput(float amount, Direction direction);

  void OnInputForward(float fDeltaTime, float amount);
  void OnInputStrafe(float fDeltaTime, float amount);
  void OnInputLookUp(float fDeltaTime, float amount);
  void OnInputLookRight(float fDeltaTime, float amount);
  void OnInputShiftSlice(float fDeltaTime);

  void printIt();

  const Mat4f& getCameraMatrix() const {
    return _cameraMatrix;
  }
  const Vec4f& getCameraPos() const {
    return _cameraPos;
  }
  Vec4f getLookForward() const {
    // The minus sign here means the author sucks.
    return -(_renderMatrix[FORWARD]);
    //return -getCameraMatrix()[FORWARD];
  }
  const Mat4f& getRenderMatrix() const {
    return _renderMatrix;
  }
  const Vec4f& getRenderPos() const {
    return _renderPos;
  }

  MovementMode getMovementMode() const { return _movement; }
  void setMovementMode(MovementMode mode);

  void* operator new(size_t size) { return _aligned_malloc(size, 16); }
  void operator delete(void* p) { _aligned_free(p); }

  void Step(float fDeltaTime) {
    // after all this component stuff,
    // now I want to not do it this way and instead do crazy things
    // like make camera a component of render and chain stuff magically...
    _componentBus.Step(fDeltaTime);
  }

  ComponentBus& GetComponentBus() { return _componentBus; }
  static void RunTests();

private:
  void RenormalizeCamera(Direction changeBasis);
  void PickBasisFromForward(const Direction changeBasis, Direction& firstOther,
      Direction& secondOther);
};

} // namespace fd
