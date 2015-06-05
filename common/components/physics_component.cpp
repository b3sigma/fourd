#include "physics_component.h"

namespace fd {

void PhysicsComponent::OnConnected() {
  static std::string BDATpos("position");
  static std::string BDATorient("orientation");
      
  if(!m_ownerBus->GetOwnerData(BDATorient, true, &m_pOwnerOrientation)
      || !m_ownerBus->GetOwnerData(BDATpos, true, &m_pOwnerPosition)) {
    assert(false);
    SelfDestruct();
  }
  RegisterSignal(std::string("Step"), this, &PhysicsComponent::OnStepSignal);
  // This is a bad sign... Should we do a typeid system?
  RegisterSignal(std::string("DestroyPhysics"), (Component*)this, &Component::SelfDestruct);
  RegisterSignal(std::string("AddImpulse"), this, &PhysicsComponent::OnImpulse);
}
    
void PhysicsComponent::OnImpulse(const Vec4f& impulse) {
  m_velocity += impulse;
}
    
void PhysicsComponent::OnStepSignal(float delta) {
  m_velocity += m_pPhysics->m_gravity * delta;

  Vec4f frameVelocity = m_velocity * delta;

  // yeah it's becoming quickly clear that a slow progression of physics bits
  // is the wrong way to proceed. Should just do a proper decoupled system
  // where there are rigidbody + shape things in a physics scene.
  Vec4f possibleVelocity;
  if(m_pShape->DoesMovementCollide(*m_pOwnerOrientation, *m_pOwnerPosition,
      m_velocity, delta, possibleVelocity)) {
    frameVelocity = possibleVelocity * delta;
    m_velocity = possibleVelocity;
  }
  *m_pOwnerPosition += frameVelocity;

  //float distance = 0.0f; 

  //if(m_pPhysics->RayCast(*m_pOwnerPosition, m_velocity, &distance)) {
  //  if(deltaPos.lengthSq() > (distance*distance)) {
  //    deltaPos = m_velocity.normalized() * (distance - m_pPhysics->m_cushion);
  //  }
  //} 
  //
  //*m_pOwnerPosition += deltaPos;
      
  // right now movement can mess things up since we're doing direct set
  // so clamp to ground
  m_pPhysics->ClampToGround(m_pOwnerPosition, &m_velocity);
}

} // namespace fd