#pragma once

#include "fourmath.h"

namespace fd {

// shape interface class
class PhysicsShape {
public:
  virtual bool DoesMovementCollide(
      const Mat4f& orientation, const Vec4f& position,
      const Vec4f& velocity, float deltaTime,
      Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal) {
    return false; 
  }
};

} // namespace fd
