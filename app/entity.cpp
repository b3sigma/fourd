#include "entity.h"

namespace fd {

Entity::Entity() 
    : _pMesh(NULL)
    , _pShader(NULL) {
  _componentBus.RegisterOwnerData(
      std::string("orientation"), &_orientation, true);
  _componentBus.RegisterOwnerData(
      std::string("position"), &_position, true);
}

bool Entity::Initialize(Mesh* pMesh, Shader* pShader) {
  if (!pMesh || !pShader) return false;

  _pMesh = pMesh;
  _pShader = pShader;
  return true;
}

} // namespace fd
