#include <stdio.h>
#include "camera.h"
#include "components/animated_rotation.h"

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
  rot.buildRotation(radians, (int)target, (int)source);
  _cameraMatrix = rot * _cameraMatrix;
}

void Camera::ApplyWorldRotation(
    float radians, Direction target, Direction source) {
  Mat4f rot;
  rot.buildRotation(radians, (int)target, (int)source);
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

void Camera::printIt() {
  printf("Camera pos:");
  _cameraPos.printIt();
  printf("\nMatrix:\n");
  _cameraMatrix.printIt();
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
  };


void TestSignals() {
  Target t1;
  assert(g_targetCount == 0);
  DelegateN<void, int> callerOne;
  callerOne.Bind(&t1, &_internal::Target::TargetFunc);
  callerOne(5);
  assert(g_targetCount == 5);

  DelegateN<int, int, int, char*> callerMore;
  callerMore.Bind(&t1, &_internal::Target::TargetMoreArgs);
  callerMore(20, 100, "blah");
  assert(g_targetCount == 125);


	//
	//// Test plain delegate
	//Delegate1< int > delegate;
	//delegate.Bind( &l, &Label::Update1 );
	//delegate( 5 );

	//// Connect a bunch of signals
	//b.update0.Connect( &l, &Label::Update0 ); // zero parameter
	//b.update2.Connect( &l, &Label::Update2 ); // two parameters
	//b.update0.Connect( &l2, &Label::Update0 ); // virtual method
	//b.update0.Connect( &Global ); // global function
	//b.update0.Connect( &Label::Static ); // static method

	//#define Connect( a, signal, b, slot ) a.signal.Connect( &b, &slot )
	//Connect( b, update1, l, Label::Update1 ); // we could do QT style with a macro
	//
	//b.ClickButton(); // emit signals
	//
	//printf( "Disconnect Update0, Update1 and Global \n" );
	//b.update0.Disconnect( &l, &Label::Update0 );
	//b.update0.Disconnect( &l2, &Label::Update0 );
	//b.update1.Disconnect( &l, &Label::Update1 );
	//b.update0.Disconnect( &Global );
	//
	//b.ClickButton(); // emit signals again, shouldn't see disconnected slots firing
}
} // namespace _internal

class SuicideComponent : public Component {
  virtual void OnConnected() {
    //_bus->RegisterSignal("Step", this, &::SuicideComponent::OnStepSignal);
    SelfDestruct();
  }

  void OnStepSignal(float delta) {
    Mat4f* mat;
    _bus->GetOwnerData("orientation", false, &mat);
    mat->storeZero(); // mess up matrix intentionally
  }
};

void Camera::TestComponents() {
  _internal::TestSignals();

  Mat4f identity;
  identity.storeIdentity();
  Camera* pCamera;
  
  pCamera = new Camera();
  assert(pCamera->_cameraMatrix == identity);
  pCamera->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI / 4.0f,
          (int)::fd::Camera::INSIDE, (int)::fd::Camera::RIGHT,
          5.0f, false));
  // Haven't stepped yet, shouldn't be different
  assert(pCamera->_cameraMatrix == identity);
  delete pCamera;

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

}

