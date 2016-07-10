#pragma once

#include "../../common/camera.h"
#include "../../common/component.h"
#include "../../common/fourmath.h"

namespace fd {

// the screensaver kills itself if it receives any input signal,
// otherwise it spins the view around
class ScreenSaverComponent : public Component {
public:
  Mat4f* m_pOwnerOrientation; // note as this is 4d we need full 4d matrix for orientation
  Vec4f* m_pOwnerPosition; // also need full vec4 for position.

  Mat4f m_randomRot;
  float m_randomTotalTime;
  float m_randomTime;

public:
  ScreenSaverComponent()
      : m_randomTotalTime(10.0f)
      , m_randomTime(0.0f) {
    m_randomRot.storeIdentity();
  }

  virtual void OnConnected() {
    static std::string BDATpos("position");
    static std::string BDATorient("orientation");

    if(!m_ownerBus->GetOwnerData(BDATorient, true, &m_pOwnerOrientation)
        || !m_ownerBus->GetOwnerData(BDATpos, true, &m_pOwnerPosition)) {
      assert(false);
      SelfDestruct();
    }

    RegisterSignal(std::string("Step"), this,
        &ScreenSaverComponent::OnStepSignal);
    RegisterSignal(std::string("AnyInput"), this,
        &ScreenSaverComponent::OnAnyInput);
  }

  void OnAnyInput() {
    m_ownerBus->SendSignal(std::string("RestartGameState"), SignalN<>());
    SelfDestruct();
  }

  void OnStepSignal(float delta) {
    m_randomTime -= delta;
    if(m_randomTime <= 0.0f) {
      m_randomTime = m_randomTotalTime;
      int randSourceAxis = rand() % 4;
      int randTargetAxis = rand() % 4;
      if(randSourceAxis == randTargetAxis) {
        randTargetAxis = (randTargetAxis + 1) % 4;
      }
      float maxRot = 0.001f;
      float randRotAmount = maxRot * (float)(rand() % 1000) / (1000.0f) * 2.0f * 3.14f;
      m_randomRot.storeRotation(randRotAmount, randTargetAxis, randSourceAxis);
    }

    *m_pOwnerPosition = m_randomRot.transform(*m_pOwnerPosition);
    *m_pOwnerOrientation = (*m_pOwnerOrientation) * m_randomRot;
  }

  ALIGNED_ALLOC_NEW_DEL_OVERRIDE
};

}; //namespace fd
