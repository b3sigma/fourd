#pragma once

#include "../component.h"
#include "../fourmath.h"
#include "../physics.h"
#include "../physics_shape_interface.h"

namespace fd {

  // The plan is to attach a physics component to something like the camera.
  // The physics component won't know what it's attached to necessarily, but it
  // should only need information about the owner's position and orientation.
  // And then the component talks to the physics system to do position updates.
  //
  // Probably should factor into fixed step and render step later, but for now just step.
  class PhysicsComponent : public Component {
  public:
    Physics* m_pPhysics; // not owned
    PhysicsShape* m_pShape; //owned

    Vec4f m_velocity;
    float m_jumpCountdown;

    Mat4f* m_pOwnerOrientation; // note as this is 4d we need full 4d matrix for orientation
    Vec4f* m_pOwnerPosition; // also need full vec4 for position.
    
    Vec4f* m_pOwnerPushVelocity; // optional, may be null
    bool*  m_pOwnerCollidingLastFrame;
  public:

    PhysicsComponent(Physics* pPhys, PhysicsShape* shape)
        : m_pPhysics(pPhys), m_pShape(shape) {}
    virtual ~PhysicsComponent() {
      delete m_pShape;
    }

    virtual void OnConnected();

    void OnJump(float frameTime); // should not be here, too specific
    void OnImpulse(const Vec4f& impulse);
    
    void OnStepSignal(float delta);
  };

}; //namespace fd
