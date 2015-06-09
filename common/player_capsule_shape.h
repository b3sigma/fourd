#pragma once

#include "fourmath.h"
#include "physics_shape_interface.h"

namespace fd {

class Physics;
  
class PlayerCapsuleShape : public PhysicsShape {
public:
  Physics* m_pPhysics;
  float m_radius;

  PlayerCapsuleShape(Physics* phys) : m_pPhysics(phys), m_radius(1.0f) {}

  virtual bool DoesMovementCollide(
      const Mat4f& orientation, const Vec4f& position,
      const Vec4f& velocity, float deltaTime,
      Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal);
};


} // namespace fd