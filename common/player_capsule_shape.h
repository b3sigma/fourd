#pragma once

#include "fourmath.h"
#include "physics_shape_interface.h"

namespace fd {

class Physics;
  
class PlayerCapsuleShape : public PhysicsShape {
public:
  Physics* m_pPhysics;
  float m_radius;
  Vec4f m_offset;

  PlayerCapsuleShape(Physics* phys, float radius, const Vec4f& offset) 
      : m_pPhysics(phys), m_radius(radius), m_offset(offset) {}

  virtual bool ApplyMovement(
      float& deltaTime, Vec4f& velocity,
      Mat4f& orientation, Vec4f& position,
      bool& hadGroundCollision);

  virtual bool DoesCollide(
      float& deltaTime,
      const Mat4f& orientation, const Vec4f& position,
      Vec4f& betterPos, Vec4f& collidePos, Vec4f& collideNormal);

  virtual bool DoesMovementCollide(
      const Mat4f& orientation, const Vec4f& position,
      const Vec4f& velocity, float& deltaTime,
      Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal);

  virtual bool UseShapeAppliedMovement() { return true; }

};


} // namespace fd