#include "stencil_portal_component.h"

#include "../../common/camera.h"
#include "../../common/physics.h"
#include "../scene.h"

namespace fd {

StencilPortalComponent::StencilPortalComponent() 
    : m_renderRecursionMax(3)
    , m_renderRecursionCurrent(0)
    , m_nextCameraCopyIndex(0)
    , m_targetOrientation(NULL)
    , m_targetPosition(NULL) {
}

StencilPortalComponent::~StencilPortalComponent() {
  for(auto pCamera : m_cameraStack) {
    delete pCamera;
  }
  m_cameraStack.empty();
  delete m_targetOrientation;
  delete m_targetPosition; 
}

void StencilPortalComponent::OnConnected() {
  static std::string BDATpos("position");
  static std::string BDATorient("orientation");
  static std::string BDATscene("scene");
      
  if(!m_ownerBus->GetOwnerData(BDATorient, true, &m_pOwnerOrientation)
      || !m_ownerBus->GetOwnerData(BDATpos, true, &m_pOwnerPosition)
      || !m_ownerBus->GetOwnerDataPtr(BDATscene, true, &m_pOwnerScene)) {
    assert(false);
    SelfDestruct();
  }

  m_renderRecursionMax = 4;
  m_targetOrientation = new Mat4f();
  m_targetOrientation->storeRotation((float)PI, 0, 1); // uh, dunno
  m_targetPosition = new Vec4f(10.0f, 10.0f, 100.0f, 10.0f);

  static fd::Vec4f shiftoffset(-120.0f, 13.0f, 17.0f, 3.0f);
  (*m_pOwnerPosition) += shiftoffset;

  //m_ownerBus->SendSignal(std::string("SetMesh"), SignalN<Mesh*>(), m_pOwnerScene->m_pQuaxolMesh);
  //m_ownerBus->SendSignal(std::string("SetShader"), SignalN<Shader*>(), m_pOwnerScene->m_pQuaxolShader);

  RegisterSignal(std::string("AfterRender"), this, &StencilPortalComponent::OnAfterRender);
}

void StencilPortalComponent::OnAfterRender(Camera* pCamera) {
  if(m_renderRecursionCurrent < m_renderRecursionMax) {
    m_renderRecursionCurrent++;

    int cameraCopyIndex = m_nextCameraCopyIndex; //make a callstack copy
    m_nextCameraCopyIndex++;
    if(cameraCopyIndex >= (int)m_cameraStack.size()) {
      m_cameraStack.emplace_back(new Camera());
      assert(cameraCopyIndex < (int)m_cameraStack.size());
    }
    
    pCamera->DuplicateStateTo(m_cameraStack[cameraCopyIndex]);
    pCamera->SetCameraPosition((*m_pOwnerPosition) + pCamera->getCameraPos());
    //pCamera->SetCameraPosition(*m_targetPosition);
    //pCamera->SetCameraOrientation(*m_targetOrientation);
    pCamera->UpdateRenderMatrix(NULL, NULL); // this looks like fucking black magic currently
    m_pOwnerScene->RenderEverything(pCamera);
    pCamera->RestoreStateFrom(m_cameraStack[cameraCopyIndex]);
    
    m_nextCameraCopyIndex--;
    assert(m_nextCameraCopyIndex == cameraCopyIndex, "Problem with recursive rendering code?");
    m_renderRecursionCurrent--;
  }
}

} // namespace fd