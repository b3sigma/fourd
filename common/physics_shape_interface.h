#pragma once

#include "fourmath.h"

namespace fd {

// shape interface class
class PhysicsShape {
public:
  virtual bool ApplyMovement(
      float& deltaTime, Vec4f& velocity,
      Mat4f& orientation, Vec4f& position,
      bool& hadGroundCollision) {
    return false;
  }

  virtual bool DoesCollide(
      float& deltaTime, //const Vec4f& prevPosition,
      const Mat4f& orientation, const Vec4f& testPosition,
      Vec4f& safePos, Vec4f& collidePos, Vec4f& collideNormal) {
    return false;
  }

  virtual bool DoesMovementCollide(
      const Mat4f& orientation, const Vec4f& position,
      const Vec4f& velocity, float& deltaTime,
      Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal) {
    return false; 
  }

  virtual bool UseShapeAppliedMovement() { return false; }
};

} // namespace fd
