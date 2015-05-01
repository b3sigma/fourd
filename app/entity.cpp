#include "entity.h"

namespace fd {

Entity::Entity() 
    : m_pMesh(NULL)
    , m_pShader(NULL) {
  m_componentBus.RegisterOwnerData(
      std::string("orientation"), &m_orientation, true);
  m_componentBus.RegisterOwnerData(
      std::string("position"), &m_position, true);
}

bool Entity::Initialize(Mesh* pMesh, Shader* pShader) {
  if (!pMesh || !pShader) return false;

  m_pMesh = pMesh;
  m_pShader = pShader;
  return true;
}

} // namespace fd
