#include "stencil_portal_component.h"

#include "../../common/camera.h"
#include "../../common/physics.h"
#include "../scene.h"

namespace fd {

StencilPortalComponent::StencilPortalComponent() 
    : m_renderRecursionMax(3)
    , m_renderRecursionCurrent(0)
    , m_nextCameraCopyIndex(0)
    , m_targetOrientation(new Mat4f())
    , m_targetPosition(new Vec4f()) {
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

  m_renderRecursionMax = 1;

  // This string shit is turning into a terrible mix of typed and untyped code, without any 
  //   debugging tools. Each component bus should have a logging capability, or maybe that's
  //   just another component that can listen to untouched messages.
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
    fd::Vec4f newCameraPos = (*m_targetPosition);
    fd::Mat4f newCameraOri = (*m_targetOrientation);
    fd::Vec4f localPortalToCamera(pCamera->getCameraPos() - (*m_pOwnerPosition));
    //newCameraPos += newCameraOri.transform(localPortalToCamera);
    newCameraPos += newCameraOri.inverse().transform(localPortalToCamera);
    pCamera->SetCameraPosition(newCameraPos);
    pCamera->SetCameraOrientation(pCamera->getCameraMatrix() * newCameraOri);
    
    //pCamera->SetCameraPosition(pCamera->getCameraPos() - (*m_pOwnerPosition) + (*m_targetPosition));
    //pCamera->SetCameraPosition(m_targetOrientation->transform(pCamera->getCameraPos()) - (*m_pOwnerPosition) 
    //    + (*m_targetPosition));
    //pCamera->SetCameraOrientation(*m_targetOrientation);

    pCamera->UpdateRenderMatrix(NULL, NULL); // this looks like fucking black magic currently
    m_pOwnerScene->RenderEverything(pCamera);
    pCamera->RestoreStateFrom(m_cameraStack[cameraCopyIndex]);
    
    m_nextCameraCopyIndex--;
    assert(m_nextCameraCopyIndex == cameraCopyIndex); //, "Problem with recursive rendering code?");
    m_renderRecursionCurrent--;
  }
}

} // namespace fd