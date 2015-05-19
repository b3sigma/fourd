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
    LOOK = 1, ORBIT = 2,
  };

  MovementMode _movement;
  Vec4f _cameraLookAt;

  ComponentBus _componentBus;

  Camera()
      : _movement(ORBIT)
      , _zNear(0.1f)
      , _zFar(1000.0f)
      , _zFov(90.0f)
      , _wNear(0.0f)
      , _wFar(40.0f)
      , _wScreenSizeRatio(0.5)
      , _wProjectionEnabled(true) {
    _cameraMatrix.storeIdentity();
    _cameraPos.storeZero();
    _renderMatrix.storeIdentity();
    _renderPos.storeZero();
    _fourToThree.storeIdentity();

    bool success = _componentBus.RegisterOwnerData(
        std::string("orientation"), &_cameraMatrix, true);
    success &= _componentBus.RegisterOwnerData(
        std::string("position"), &_cameraPos, true);
    assert(success == true);
  }

  Camera(MovementMode mode)
      : _movement(mode) {
    _cameraMatrix.storeIdentity();
    _cameraPos.storeZero();
  }

  void NoOffsetUpdate() {
    _renderMatrix = _cameraMatrix;
    _renderPos = _cameraPos;
  }

  void SetZProjection(int width, int height, 
      float zFov, float zNear, float zFar) {

    _screenBounds.x() = width;
    _screenBounds.y() = height;
    _zFov = zFov;
    _zNear = zNear;
    _zFar = zFar;
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    _zProjectionMatrix.store3dProjection(_zFov, aspect, _zNear, _zFar);
  }

  void SetWProjection(float wNear, float wFar, float wScreenSizeRatio) {
    _wNear = wNear;
    _wFar = wFar;
    _wScreenSizeRatio = wScreenSizeRatio;
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
  Vec4f getCameraForward() const {
    // The minus sign here means the author sucks.
    return -getCameraMatrix()[FORWARD];
  }
  const Mat4f& getRenderMatrix() const {
    return _renderMatrix;
  }
  const Vec4f& getRenderPos() const {
    return _renderPos;
  }

  MovementMode getMovementMode() const { return _movement; }
  void setMovementMode(MovementMode mode) {
    _movement = mode;
  }

  void* operator new(size_t size) { return _aligned_malloc(size, 16); }
  void operator delete(void* p) { _aligned_free(p); }

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
