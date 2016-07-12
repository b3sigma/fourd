#include "entity.h"
#include "texture.h"
#include "scene.h"

namespace fd {

Entity::Entity(Scene* scene)
    : m_pMesh(NULL)
    , m_pShader(NULL)
    , m_scene(scene)
{
  m_componentBus.RegisterOwnerData(
      std::string("orientation"), &m_orientation, true);
  m_componentBus.RegisterOwnerData(
      std::string("position"), &m_position, true);
  m_componentBus.RegisterOwnerDataPtr(
      std::string("scene"), &m_scene, true);
  m_componentBus.RegisterOwnerDataPtr(
      std::string("mesh"), &m_pMesh, true);
  m_componentBus.RegisterOwnerDataPtr(
      std::string("shader"), &m_pShader, true);

  m_componentBus.RegisterSignal(
      std::string("DeleteSelf"), this, &Entity::OnDeleteSelf);
  m_componentBus.RegisterSignal(
      std::string("SetMesh"), this, &Entity::SetMesh);
  m_componentBus.RegisterSignal(
      std::string("SetShader"), this, &Entity::SetShader);

  m_orientation.storeIdentity();
  m_position.storeZero();
}

Entity::~Entity() {
  if(m_ownerBus) {
    m_ownerBus->SendSignal(std::string("EntityDeleted"),
        SignalN<Entity*>(), this);
  }
}

void Entity::OnConnected() {
  // Should do a pass through for signals down to the bus.
  RegisterSignal(std::string("Step"), this, &Entity::OnStepSignal);
}

void Entity::SetMesh(Mesh* pMesh) {
  m_pMesh = pMesh; //so it's not owned anyway, I guess this is fine.
}

void Entity::SetShader(Shader* pShader) {
  m_pShader = pShader; //so it's not owned anyway, I guess this is fine.
}

// try the manual thing first
void Entity::OnStepSignal(float delta) {
  m_componentBus.Step(delta);
}

void Entity::OnDeleteSelf() {
  m_ownerBus->SendSignal(std::string("DeleteEntity"),
      SignalN<Entity*>(), this);
}

bool Entity::Initialize(Mesh* pMesh, Shader* pShader,
      const TTextureList* pTextures) {
  if (!pMesh || !pShader) return false;

  m_pMesh = pMesh;
  m_pShader = pShader;

  if(pTextures) {
    m_textures.assign(pTextures->begin(), pTextures->end());
  }
  return true;
}

} // namespace fd
