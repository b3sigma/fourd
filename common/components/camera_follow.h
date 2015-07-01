#pragma once

#include "../../common/camera.h"
#include "../../common/component.h"
#include "../../common/fourmath.h"

namespace fd {

class CameraFollowComponent : public Component {
public:
  Mat4f* m_pOwnerOrientation; // note as this is 4d we need full 4d matrix for orientation
  Vec4f* m_pOwnerPosition; // also need full vec4 for position.
  Camera* m_camera;

public:
  CameraFollowComponent(Camera* camera) : m_camera(camera) {
    assert(m_camera != NULL);
  }

  virtual void OnConnected() {
    static std::string BDATpos("position");
    static std::string BDATorient("orientation");
      
    if(!m_ownerBus->GetOwnerData(BDATorient, true, &m_pOwnerOrientation)
        || !m_ownerBus->GetOwnerData(BDATpos, true, &m_pOwnerPosition)) {
      assert(false);
      SelfDestruct();
    }

    RegisterSignal(std::string("Step"), this, &CameraFollowComponent::OnStepSignal);
  }

  // fuck, now is the time to split this into physicsStep and somethingOtherStep
  // otherwise there will be an ordering issue
  void OnStepSignal(float delta) {
    *m_pOwnerPosition = m_camera->getCameraPos();
    *m_pOwnerOrientation = m_camera->getRenderMatrix();
  }
};

}; //namespace fd
