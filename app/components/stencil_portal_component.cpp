#include "stencil_portal_component.h"

#include "../../common/camera.h"
#include "../../common/physics.h"
#include "../scene.h"

namespace fd {

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

  static fd::Vec4f shiftoffset(12.0f, 13.0f, 17.0f, 3.0f);
  (*m_pOwnerPosition) += shiftoffset;

  m_ownerBus->SendSignal(std::string("SetMesh"), SignalN<Mesh*>(), m_pOwnerScene->m_pQuaxolMesh);
  m_ownerBus->SendSignal(std::string("SetShader"), SignalN<Shader*>(), m_pOwnerScene->m_pQuaxolShader);

  RegisterSignal(std::string("afterRender"), this, &StencilPortalComponent::OnAfterRender);
}

void StencilPortalComponent::OnAfterRender() {
}

} // namespace fd