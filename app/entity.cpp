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
  m_componentBus.RegisterOwnerData(
      std::string("velocity"), &m_velocity, true);
  m_componentBus.RegisterOwnerDataPtr(
      std::string("scene"), &m_scene, true);

  m_componentBus.RegisterSignal(
      std::string("DeleteSelf"), this, &Entity::OnDeleteSelf);

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
