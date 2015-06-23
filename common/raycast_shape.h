#pragma once

#include <vector>
#include "fourmath.h"
#include "physics_shape_interface.h"

namespace fd {

// Before doing legit 4d physics, try a simple collection of raycasts with
// impulse responses, which makes a shitty approximation to a shape depending
// on how many rays there are.
class Physics;

class BoundingBox {
public:
  Vec4f m_min;
  Vec4f m_max;
};

class RaycastShape : public PhysicsShape {
public:
  Physics* m_pPhysics;

  // not normalized, length included
  typedef std::vector<Vec4f> RayList;
  RayList m_rays;

  RaycastShape(Physics* phys) : m_pPhysics(phys) {}

  void AddCapsuleRays(float size);

  virtual bool DoesCollide(
      float& deltaTime,
      const Mat4f& orientation, const Vec4f& position,
      Vec4f& hitPos, Vec4f& hitNormal);

  virtual bool DoesMovementCollide(
      const Mat4f& orientation, const Vec4f& position,
      const Vec4f& velocity, float& deltaTime,
      Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal); 
};


} // namespace fd