#pragma once

#include "../component.h"
#include "../fourmath.h"
#include "../physics.h"

namespace fd {

  // The plan is to attach a physics component to something like the camera.
  // The physics component won't know what it's attached to necessarily, but it
  // should only need information about the owner's position and orientation.
  // And then the component talks to the physics system to do position updates.
  //
  // Probably should factor into fixed step and render step later, but for now just step.
  class PhysicsComponent : public Component {
  public:
    Physics* m_pPhysics;

    Vec4f m_velocity;

    Mat4f* m_pOwnerOrientation; // note as this is 4d we need full 4d matrix for orientation
    Vec4f* m_pOwnerPosition; // also need full vec4 for position.
  public:

    PhysicsComponent(Physics* pPhys) : m_pPhysics(pPhys) {}
    virtual ~PhysicsComponent() {}

    virtual void OnConnected() {
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
    
    void OnImpulse(Vec4f impulse) {
      m_velocity += impulse;
    }
    
    void OnStepSignal(float delta) {
      m_velocity += m_pPhysics->m_gravity * delta;
      Vec4f deltaPos = m_velocity * delta;
      float distance = 0.0f; 
      if(m_pPhysics->RayCast(*m_pOwnerPosition, m_velocity, &distance)) {
        if(deltaPos.lengthSq() > (distance*distance)) {
          deltaPos = m_velocity.normalized() * (distance - m_pPhysics->m_cushion);
        }
      } 
      
      *m_pOwnerPosition += deltaPos;
      
      // right now movement can mess things up since we're doing direct set
      // so clamp to ground
      m_pPhysics->ClampToGround(m_pOwnerPosition);
    }
  };

}; //namespace fd
