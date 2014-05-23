#include <stdio.h>
#include "camera.h"

using namespace ::fd;

void Camera::ApplyRotationInput(float radians, Direction target, Direction source) {
  if (_movement == LOOK) {
    Mat4f rot;
    rot.buildRotation(radians, (int)target, (int)source);
    _cameraMatrix = rot * _cameraMatrix;
  }
}

void Camera::ApplyOrbitInput(float radians, Direction direction)
{
  Direction coDirection = FORWARD;
  Mat4f rot;
  rot.buildRotation(radians, (int)direction, (int)coDirection);
  Mat4f invRot;
  invRot.buildRotation(-radians, (int)direction, (int)coDirection);

  Mat4f invCamera = _cameraMatrix.inverse();
  Vec4f lookDir = _cameraPos - _cameraLookAt;
  Vec4f cameraLookDir = invCamera.transform(lookDir);
  cameraLookDir = rot.transform(cameraLookDir);
  Vec4f worldLook = _cameraMatrix.transform(cameraLookDir);
  _cameraPos = _cameraLookAt + worldLook;

  _cameraMatrix = rot * _cameraMatrix;
//  _cameraMatrix = invRot * _cameraMatrix;
//  Vec4f lookAt = _cameraPos - _cameraLookAt;
//  lookAt.storeNormalized();
//  _cameraMatrix.setRow(FORWARD, lookAt);
//  _cameraMatrix.storeOrthognoal(FORWARD, changeBasis, goodBasis,
//      goodCoBasis);

}

void Camera::ApplyRollInput(float radians, Direction target, Direction source) {
  Mat4f rot;
  rot.buildRotation(radians, (int)target, (int)source);
  _cameraMatrix = rot * _cameraMatrix;
}

void Camera::RenormalizeCamera(Direction changeBasis) {
  Vec4f lookAt = _cameraPos - _cameraLookAt;
  lookAt.storeNormalized();
  _cameraMatrix.setRow(FORWARD, lookAt);

  Direction goodBasis;
  Direction goodCoBasis;
  switch (changeBasis) {
    case UP:
      goodBasis = INSIDE;
      goodCoBasis = RIGHT;
      break;
    case RIGHT:
      goodBasis = UP;
      goodCoBasis = INSIDE;
      break;
    case INSIDE:
      goodBasis = RIGHT;
      goodCoBasis = UP;
      break;
    default:
    case FORWARD:
      assert(false);
      break;
  }
  _cameraMatrix.storeOrthognoal(FORWARD, changeBasis, goodBasis,
      goodCoBasis);
}

void Camera::ApplyTranslationInput(float amount, Direction direction) {
  if (_movement == ORBIT) {
    static bool angular = false;
    if (angular) {
      if (direction == FORWARD) {
        _cameraPos += _cameraMatrix[direction] * amount;
      }
      else {
        static float rotationScalar = 2 * PI / 100.0f;
        ApplyOrbitInput(amount * rotationScalar, direction);
      }
    } else {
      Vec4f lookAt = _cameraPos - _cameraLookAt;
      float distToLook = lookAt.length();
      _cameraPos += _cameraMatrix[direction] * amount;
      if (direction != FORWARD) {
        _cameraPos.storeNormalized();
        _cameraPos = _cameraPos * distToLook;
        RenormalizeCamera(direction);
      }
    }
  } else {
    _cameraPos += _cameraMatrix[direction] * amount;
  }
}

void Camera::printIt() {
  printf("Camera pos:");
  _cameraPos.printIt();
  printf("\nMatrix:");
  _cameraMatrix.printIt();
}
