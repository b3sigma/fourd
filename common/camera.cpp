#include <stdio.h>
#include "camera.h"
#include "components/animated_rotation.h"
#include "components/animated_camera_params.h"

using namespace ::fd;

Camera::Camera()
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
  success &= _componentBus.RegisterOwnerData(
      std::string("wNear"), &_wNear, true);
  success &= _componentBus.RegisterOwnerData(
      std::string("wFar"), &_wFar, true);
  success &= _componentBus.RegisterOwnerData(
      std::string("wScreenSizeRatio"), &_wScreenSizeRatio, true);
  assert(success == true);
}

void Camera::setMovementMode(MovementMode mode) {
  _movement = mode;
}

void Camera::ApplyRotationInput(float radians, Direction target, Direction source) {
  if (_movement == LOOK || _movement == WALK) {
    Mat4f rot;
    rot.storeRotation(radians, (int)target, (int)source);
    _cameraMatrix = rot * _cameraMatrix;
  }
}

void Camera::ApplyOrbitInput(float radians, Direction direction)
{
  Direction coDirection = FORWARD;
  Mat4f rot;
  rot.storeRotation(radians, (int)direction, (int)coDirection);
  Mat4f invRot;
  invRot.storeRotation(-radians, (int)direction, (int)coDirection);

  Mat4f invCamera = _cameraMatrix.inverse();
  Vec4f lookDir = _cameraPos - _cameraLookAt;
  Vec4f cameraLookDir = invCamera.transform(lookDir);
  cameraLookDir = rot.transform(cameraLookDir);
  Vec4f worldLook = _cameraMatrix.transform(cameraLookDir);
  _cameraPos = _cameraLookAt + worldLook;

  _cameraMatrix = _cameraMatrix * invRot;

//  Vec4f lookAt = _cameraPos - _cameraLookAt;
//  lookAt.storeNormalized();
//  _cameraMatrix.setRow(FORWARD, lookAt);
//  Direction firstInvariant;
//  Direction secondInvariant;
//  PickBasisFromForward(direction, firstInvariant, secondInvariant);
//  _cameraMatrix.storeOrthognoal(FORWARD, direction, firstInvariant,
//      secondInvariant);
}

void Camera::PickBasisFromForward(const Direction changeBasis, Direction& firstOther, Direction& secondOther) {
  switch (changeBasis) {
    case UP: // f,u,r,i (3,2,1,4) -> -
      firstOther = RIGHT;
      secondOther = INSIDE;
      break;
    case RIGHT: // f,r,i,u (3,1,4,2) -> -
      firstOther = INSIDE;
      secondOther = UP;
      break;
    case INSIDE: // f,i,u,r (3,4,2,1) -> -
      firstOther = UP;
      secondOther = RIGHT;
      break;
    default:
    case FORWARD:
      assert(false);
      break;
  }
}

void Camera::ApplyRollInput(float radians, Direction target, Direction source) {
  Mat4f rot;
  rot.storeRotation(radians, (int)target, (int)source);
  _cameraMatrix = rot * _cameraMatrix;
}

void Camera::ApplyWorldRotation(
    float radians, Direction target, Direction source) {
  Mat4f rot;
  rot.storeRotation(radians, (int)target, (int)source);
  _cameraMatrix = _cameraMatrix * rot.transpose();
}

void Camera::RenormalizeCamera(Direction changeBasis) {
  Vec4f lookAt = _cameraPos - _cameraLookAt;
  lookAt.storeNormalized();
  _cameraMatrix.setRow(FORWARD, lookAt);

  Direction goodBasis;
  Direction goodCoBasis;
  PickBasisFromForward(changeBasis, goodBasis, goodCoBasis);
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
        static float rotationScalar = 2.0f * (float)PI / 100.0f;
        ApplyOrbitInput(amount * rotationScalar, direction);
      }
    } else {
      Vec4f lookAt = _cameraPos - _cameraLookAt;
      float distToLook = lookAt.length();
      _cameraPos += _cameraMatrix[direction] * amount * 10.0f;
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

void Camera::SetWProjection(float wNear, float wFar, float wScreenSizeRatio, float animateTime) {
  if(animateTime > 0.0f) {
    GetComponentBus().AddComponent(new AnimatedCameraParams(wNear, wFar, wScreenSizeRatio, animateTime));
  } else {
    _wNear = wNear;
    _wFar = wFar;
    _wScreenSizeRatio = wScreenSizeRatio;
  }
}


void Camera::printIt() {
  printf("Camera pos:");
  _cameraPos.printIt();
  printf("\nMatrix:\n");
  _cameraMatrix.printIt();
  printf("zN:%f zF:%f zFov:%f\n", _zNear, _zFar, _zFov);
  printf("wN:%f wF:%f wRatio:%f\n", _wNear, _wFar, _wScreenSizeRatio);
}

namespace _internal {
  static int g_targetCount = 0;
  class Target {
  public:
    void TargetFunc(int count) {
      g_targetCount += count;
    }

    int TargetMoreArgs(int count, int useless, char* stuff) {
      g_targetCount += count + useless;
      return g_targetCount;
    }

    void DifferentOneParam(int number) {
      g_targetCount -= number - 1;
    }
  };

  class Slotty {
  public:
    void SlotFunc(int count) {
      g_targetCount++;
    }
  };

void TestSignals() {
  Target t1;
  g_targetCount = 0;
  DelegateN<void, int> callerOne;
  callerOne.Bind(&t1, &_internal::Target::TargetFunc);
  callerOne(5);
  assert(g_targetCount == 5);

  g_targetCount = 0;
  DelegateN<int, int, int, char*> callerMore;
  callerMore.Bind(&t1, &_internal::Target::TargetMoreArgs);
  callerMore(20, 100, "blah");
  assert(g_targetCount == 120);

  Slotty slotty;
  SignalN<int> signaler;
  signaler.Connect(&t1, &Target::TargetFunc);
  signaler.Connect(&t1, &Target::DifferentOneParam);
  signaler.Connect(&slotty, &Slotty::SlotFunc);
  g_targetCount = 0;
  signaler.Emit(5);
  assert(g_targetCount == 2);
  signaler.Disconnect(&t1, &Target::DifferentOneParam);
  signaler.Emit(5);
  assert(g_targetCount == 8);
}
} // namespace _internal

class SuicideComponent : public Component {
  virtual void OnConnected() {
    RegisterSignal(std::string("Step"), this, &::SuicideComponent::OnStepSignal);
    SelfDestruct();
  }

  void OnStepSignal(float delta) {
    Mat4f* mat;
    m_ownerBus->GetOwnerData(std::string("orientation"), false, &mat);
    mat->storeZero(); // mess up matrix intentionally
  }
};

void Camera::TestComponents() {
  _internal::TestSignals();

  Mat4f identity;
  identity.storeIdentity();
  Camera* pCamera;
  
  //pCamera = new Camera();
  //assert(pCamera->_cameraMatrix == identity);
  //pCamera->GetComponentBus().AddComponent(
  //    new AnimatedRotation((float)PI * 2.0f,
  //        (int)::fd::Camera::INSIDE, (int)::fd::Camera::RIGHT,
  //        5.0f, false));
  //// Haven't stepped yet, shouldn't be different
  //assert(pCamera->_cameraMatrix == identity);
  //delete pCamera;

  pCamera = new Camera();
  assert(pCamera->_cameraMatrix == identity);
  pCamera->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI * 2.0f,
          (int)::fd::Camera::INSIDE, (int)::fd::Camera::RIGHT,
          5.0f, false));
  pCamera->GetComponentBus().Step(2.5f);
  assert(pCamera->_cameraMatrix != identity);
  pCamera->GetComponentBus().Step(3.0f);
  // Should be back to identity
  assert(pCamera->_cameraMatrix == identity);
  delete pCamera;

  pCamera = new Camera();
  assert(pCamera->_cameraMatrix == identity);
  pCamera->GetComponentBus().AddComponent(
      new SuicideComponent());
  pCamera->GetComponentBus().Step(2.5f);
  // SuicideComponent messes up camera matrix if it steps
  // but since it suicided, it shouldn't have been allowed to step
  assert(pCamera->_cameraMatrix == identity);
  delete pCamera;

  pCamera = new Camera();
  assert(pCamera->_cameraMatrix == identity);
  pCamera->GetComponentBus().AddComponent(
      new SuicideComponent());
  pCamera->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI * 2.0f,
          (int)::fd::Camera::INSIDE, (int)::fd::Camera::RIGHT,
          5.0f, false));
  pCamera->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI * 2.0f,
          (int)::fd::Camera::INSIDE, (int)::fd::Camera::RIGHT,
          5.0f, false));
  pCamera->GetComponentBus().Step(2.5f);
  assert(pCamera->_cameraMatrix == identity);
  pCamera->GetComponentBus().Step(2.5f);
  assert(pCamera->_cameraMatrix == identity);
  delete pCamera;

}

