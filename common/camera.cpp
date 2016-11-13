#include <stdio.h>
#include "camera.h"
#include "tweak.h"
#include "components/animated_rotation.h"
#include "components/animated_camera_params.h"

using namespace ::fd;

Camera::Camera()
    : _zNear(0.1f)
    , _zFar(1000.0f)
    , _zFov(90.0f)
    , _wNear(0.0f)
    , _wFar(40.0f)
    , _wScreenSizeRatio(0.5)
    , _wProjectionEnabled(true)
    , _movement(ORBIT)
    , _yaw(0.0f)
    , _pitch(0.0f)
    , _collidingLastFrame(false)
    , _nextSimpleSlicePositive(true)
    , _startingCameraCopy(NULL)
    , _restartedGame(false)
{
  _cameraMatrix.storeIdentity();
  _cameraPos.storeZero();
  _renderMatrix.storeIdentity();
  _renderPos.storeZero();
  _fourToThree.storeIdentity();
  _yawPitchTrans.storeIdentity();
  _pushVelocity.storeZero();
  _velocity.storeZero();

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
  success &= _componentBus.RegisterOwnerData(
      std::string("pushVelocity"), &_pushVelocity, true);
  success &= _componentBus.RegisterOwnerData(
      std::string("collidingLastFrame"), &_collidingLastFrame, true);
  success &= _componentBus.RegisterOwnerData(
      std::string("velocity"), &_velocity, true);

  _componentBus.RegisterSignal(
      std::string("inputForward"), this, &Camera::OnInputForward);
  _componentBus.RegisterSignal(
      std::string("inputInside"), this, &Camera::OnInputInside);
  _componentBus.RegisterSignal(
      std::string("inputStrafe"), this, &Camera::OnInputStrafe);
  _componentBus.RegisterSignal(
      std::string("inputLookUp"), this, &Camera::OnInputLookUp);
  _componentBus.RegisterSignal(
      std::string("inputLookRight"), this, &Camera::OnInputLookRight);
  _componentBus.RegisterSignal(
      std::string("inputShiftSlice"), this, &Camera::OnInputShiftSlice);
  _componentBus.RegisterSignal(
      std::string("inputRoll"), this, &Camera::OnInputRoll);

  _componentBus.RegisterSignal(
      std::string("RestartGameState"), this, &Camera::RestartGameState);

  assert(success == true);
}

Camera::~Camera() {
  delete _startingCameraCopy;
}

void Camera::RestartGameState() {
  _restartedGame = true;
  if(!_startingCameraCopy) {
    return;
  }
  _cameraMatrix = _startingCameraCopy->_cameraMatrix;
  _cameraPos = _startingCameraCopy->_cameraPos;
  _movement = _startingCameraCopy->_movement;
  _cameraLookAt = _startingCameraCopy->_cameraLookAt;
  _yaw = _startingCameraCopy->_yaw;
  _pitch = _startingCameraCopy->_pitch;
  _yawPitchTrans = _startingCameraCopy->_yawPitchTrans;
  _yawTrans = _startingCameraCopy->_yawTrans;
  _pushVelocity = _startingCameraCopy->_pushVelocity;
  _collidingLastFrame = _startingCameraCopy->_collidingLastFrame;
  _velocity = _startingCameraCopy->_velocity;
}

void Camera::MarkStartingPosition() {
  _startingCameraCopy = new Camera();
  _startingCameraCopy->_cameraMatrix = _cameraMatrix;
  _startingCameraCopy->_cameraPos = _cameraPos;
  _startingCameraCopy->_movement = _movement;
  _startingCameraCopy->_cameraLookAt = _cameraLookAt;
  _startingCameraCopy->_yaw = _yaw;
  _startingCameraCopy->_pitch = _pitch;
  _startingCameraCopy->_yawPitchTrans = _yawPitchTrans;
  _startingCameraCopy->_yawTrans = _yawTrans;
  _startingCameraCopy->_pushVelocity = _pushVelocity;
  _startingCameraCopy->_collidingLastFrame = _collidingLastFrame;
  _startingCameraCopy->_velocity = _velocity;
}

void Camera::setMovementMode(MovementMode mode) {
  _movement = mode;
  if(_movement != MovementMode::WALK) {
    _yawPitchTrans.storeIdentity();
  } else {
    RebuildOrientationFromYawPitch();
    _cameraMatrix.storeIdentity();
  }
}

void Camera::UpdateRenderMatrix(Pose4f* pose) {
  if(pose) {
    UpdateRenderMatrix(&pose->rotation, &pose->position);
  } else {
    UpdateRenderMatrix(NULL /*look*/, NULL /*pos*/);
  }
}

void Camera::UpdateRenderMatrix(Mat4f* lookOffset, Vec4f* posOffset) {
  if(_movement == MovementMode::ROOM && lookOffset && posOffset) {
    // so in this case look and pos work like
    // M * x = M.R * x + M.P
    static int shotgunProgramming = 1;
    switch(shotgunProgramming) {
      case 0: {
        _renderMatrix = _cameraMatrix * (*lookOffset);
        _renderPos = _cameraMatrix.transform(*posOffset) + _cameraPos;
      } break;
      case 1: {
        _renderMatrix = (*lookOffset) * _cameraMatrix;
        _renderPos = _cameraMatrix.transform(*posOffset) + _cameraPos;
      } break;
      case 2: {
        _renderMatrix = (*lookOffset) * _cameraMatrix;
        _renderPos = *posOffset + (*lookOffset).transform(_cameraPos);
      } break;
      case 3: {
        _renderMatrix = _cameraMatrix * (*lookOffset);
        _renderPos = *posOffset + (*lookOffset).transform(_cameraPos);
      } break;
      case 4: {
        _renderMatrix = (*lookOffset) * _cameraMatrix;
        _renderPos = *posOffset + _cameraPos;
      } break;
      case 10: {
        _renderMatrix = (*lookOffset);
        _renderPos = *posOffset;
      } break;
      }
  } else {
    if(lookOffset && posOffset) {
      //_renderMatrix = (*lookOffset) * (_yawPitchTrans * _cameraMatrix);
      _renderMatrix = (*lookOffset) * (_yawTrans * _cameraMatrix);
      _renderPos = _cameraPos + *posOffset;
    } else {
      _renderMatrix = _yawPitchTrans * _cameraMatrix;
      _renderPos = _cameraPos;
    }
  }
}

void Camera::RebuildOrientationFromYawPitch() {
  _yawTrans.storeRotation(_yaw, (int)Camera::FORWARD, (int)Camera::RIGHT);
  Mat4f pitchRot;
  pitchRot.storeRotation(_pitch,  (int)Camera::FORWARD, (int)Camera::UP);

  _yawPitchTrans = pitchRot * _yawTrans;
}

void Camera::ApplyYawInput(float radians) {
  _yaw += radians;

  RebuildOrientationFromYawPitch();
}

void Camera::ApplyPitchInput(float radians) {
  _pitch += radians;
  const float maxPitch = (float)PI * 0.48f;
  const float minPitch = -maxPitch;
  if(_pitch > maxPitch) {
    _pitch = maxPitch;
  } else if(_pitch < minPitch) {
    _pitch = minPitch;
  }

  RebuildOrientationFromYawPitch();
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

// so options include adding physics to the camera in a more direct way
//  this is probably simple as we can just make the input write to a velocity
//  and then make the phys component add that velocity per frame
//  lets do the simple thing first
// or making an entity,
//  then attaching to the entity a physics component
//  then attaching an input handler thing, which maps commands to velocity?
//  then adding friction to the movement
//  then adding a hook at global level that translates app keypresses to
//    input handler commands

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
  } else if (_movement == WALK) {
    static float acceleration = 10.0f;
    static float maxWalkSpeed = 50.0f;
    static float maxAirSpeed = 30.0f;
    static float airMoveMultiplier = 0.15f;

    //float sign = (amount >= 0) ? 1.0f : -1.0f;
    float currentSpeed = _velocity.length();
    float trySpeed;
    float maxSpeed;
    if(_collidingLastFrame) {
      trySpeed = amount * acceleration;
      maxSpeed = maxWalkSpeed;
    } else {
      trySpeed = amount * acceleration * airMoveMultiplier;
      maxSpeed = maxAirSpeed;
    }
    float maxScalar = (::std::max)(maxSpeed - currentSpeed, 0.0f) / maxSpeed;
    //addSpeed *= sign;
    float addSpeed = trySpeed * maxScalar;

    //Vec4f newPushVel = _pushVelocity + (_renderMatrix[direction] * addSpeed);

    _pushVelocity += _renderMatrix[direction] * addSpeed;
    //_pushVelocity += _renderMatrix[direction] * (amount * acceleration * airMoveMultiplier);

    //_cameraPos += _renderMatrix[direction] * amount;
  } else {
    _cameraPos += _cameraMatrix[direction] * amount;
  }
}

TweakVariable runSpeedVar("game.runSpeed", 30.0f);
void Camera::OnInputForward(float fDeltaTime, float amount) {
  ApplyTranslationInput(runSpeedVar.AsFloat() * amount * fDeltaTime, Camera::FORWARD);
}

void Camera::OnInputInside(float fDeltaTime, float amount) {
  ApplyTranslationInput(runSpeedVar.AsFloat() * amount * fDeltaTime, Camera::INSIDE);
}

TweakVariable runStrafeSpeedVar("game.runStrafeSpeed", 30.0f);
void Camera::OnInputStrafe(float fDeltaTime, float amount) {
  ApplyTranslationInput(runStrafeSpeedVar.AsFloat() * amount * fDeltaTime, Camera::RIGHT);
}

void Camera::OnInputLookUp(float fDeltaTime, float amount) {
  const float joySensitivity = 2.0f;
  ApplyPitchInput(joySensitivity * fDeltaTime * amount);
}

void Camera::OnInputLookRight(float fDeltaTime, float amount) {
  const float joySensitivity = 2.0f;
  ApplyYawInput(joySensitivity * fDeltaTime * amount);
}

void Camera::OnInputShiftSlice(float fDeltaTime) {
  float sliceRotateAmount = (float)PI * 0.5f;
  if(!_nextSimpleSlicePositive) {
    sliceRotateAmount *= -1.0f;
  }

  GetComponentBus().AddComponent(
      new AnimatedRotation(sliceRotateAmount,
          (int)::fd::Camera::INSIDE, (int)::fd::Camera::RIGHT,
          2.0f /* duration */, true /* worldSpace */));

  _nextSimpleSlicePositive = !_nextSimpleSlicePositive;
}

void Camera::OnInputRoll(float fDeltaTime, float amount) {
  static float rollSensitivity = 0.01f;
  ApplyRollInput(rollSensitivity * amount, Camera::INSIDE, Camera::RIGHT);
}

void Camera::SetZProjection(int width, int height,
    float zFov, float zNear, float zFar) {

  _screenBounds.x() = width;
  _screenBounds.y() = height;
  _zFov = zFov;
  _zNear = zNear;
  _zFar = zFar;
  float aspect = static_cast<float>(width) / static_cast<float>(height);
  _zProjectionMatrix.store3dProjection(_zFov, aspect, _zNear, _zFar);
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
  char blah[] = "blah";
  callerMore(20, 100, blah);
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

class CountdownDeathComponent : public Component {
public:
  int _ticksToDie;
  CountdownDeathComponent(int count) : _ticksToDie(count) {}

  virtual void OnConnected() {
    RegisterSignal(std::string("AnySignal"), this, &::CountdownDeathComponent::OnAnySignal);
    RegisterSignal(std::string("Step"), this, &::CountdownDeathComponent::OnStepSignal);
  }
  void OnStepSignal(float delta) {
    _ticksToDie--;
    if(_ticksToDie <= 0) {
      SelfDestruct();
    }
  }

  void OnAnySignal() {
    _ticksToDie--;
    if(_ticksToDie <= 0) {
      SelfDestruct();
    }
  }
};

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

void Camera::RunTests() {
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

  // Tests for signaling and destruction
  pCamera = new Camera();
  pCamera->GetComponentBus().AddComponent(new CountdownDeathComponent(1));
  pCamera->GetComponentBus().AddComponent(new CountdownDeathComponent(2));
  pCamera->GetComponentBus().AddComponent(new CountdownDeathComponent(4));
  pCamera->GetComponentBus().AddComponent(new CountdownDeathComponent(2));
  pCamera->Step(0.1f);
  pCamera->GetComponentBus().SendSignal("AnySignal", SignalN<>());
  pCamera->Step(0.1f);
  pCamera->GetComponentBus().SendSignal("AnySignal", SignalN<>());
  pCamera->Step(0.1f);
  delete pCamera;

}
