#include "scene_modifier.h"

#include "../../common/camera.h"
#include "../../common/physics.h"
#include "../scene.h"

namespace fd {

void SceneModifierComponent::OnConnected() {
  static std::string BDATpos("position");
  static std::string BDATorient("orientation");
  static std::string BDATscene("scene");
      
  if(!m_ownerBus->GetOwnerData(BDATorient, true, &m_pOwnerOrientation)
      || !m_ownerBus->GetOwnerData(BDATpos, true, &m_pOwnerPosition)
      || !m_ownerBus->GetOwnerDataPtr(BDATscene, true, &m_pOwnerScene)) {
    assert(false);
    SelfDestruct();
  }

  RegisterSignal(std::string("inputAddQuaxol"), this, &SceneModifierComponent::OnAddQuaxol);
  RegisterSignal(std::string("inputRemoveQuaxol"), this, &SceneModifierComponent::OnRemoveQuaxol);
  RegisterSignal(std::string("inputNextCurrentItem"), this, &SceneModifierComponent::OnNextCurrentItem);
  RegisterSignal(std::string("inputPrevCurrentItem"), this, &SceneModifierComponent::OnPrevCurrentItem);
}

void SceneModifierComponent::OnAddQuaxol(float frameTime) {
  Vec4f& position = *m_pOwnerPosition;
  Vec4f ray = -(*m_pOwnerOrientation)[Camera::FORWARD];
  ray *= 1000.0f;

  QuaxolSpec gridPos;
  if(m_pOwnerScene->m_pPhysics->RayCastToOpenQuaxol(
      position, ray, &gridPos, NULL /*hitPos*/)) {
    m_pOwnerScene->SetQuaxolAt(gridPos, true /*present*/, m_currentQuaxolType);
    printf("Added quaxol at x:%d y:%d z:%d w:%d\n",
        gridPos.x, gridPos.y, gridPos.z, gridPos.w);
  }
}

void SceneModifierComponent::OnRemoveQuaxol(float frameTime) {
  Vec4f& position = *m_pOwnerPosition;
  Vec4f ray = -(*m_pOwnerOrientation)[Camera::FORWARD];
  ray *= 1000.0f;

  QuaxolSpec gridPos;
  if(m_pOwnerScene->m_pPhysics->RayCastToPresentQuaxol(
      position, ray, &gridPos, NULL /*hitPos*/)) {
    m_pOwnerScene->SetQuaxolAt(gridPos, false /*present*/, m_currentQuaxolType);
    printf("Removed quaxol at x:%d y:%d z:%d w:%d\n",
        gridPos.x, gridPos.y, gridPos.z, gridPos.w);
  }

}

static const int g_numValidTypes = 3;
void SceneModifierComponent::OnNextCurrentItem(float frameTime) {
  m_currentQuaxolType = (m_currentQuaxolType + 1 + g_numValidTypes) % g_numValidTypes; 
}

void SceneModifierComponent::OnPrevCurrentItem(float frameTime) {
  m_currentQuaxolType = (m_currentQuaxolType - 1 + g_numValidTypes) % g_numValidTypes; 
}


} // namespace fd