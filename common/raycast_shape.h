#pragma once

#include <vector>

namespace fd {

// Before doing legit 4d physics, try a simple collection of raycasts with
// impulse responses, which makes a shitty approximation to a shape depending
// on how many rays there are.
class Physics;

// shape interface class
class PhysicsShape {
public:
  virtual bool DoesMovementCollide(
      const Mat4f& orientation, const Vec4f& position,
      const Vec4f& velocity, float deltaTime, Vec4f& outVelocity) {
    return false; 
  }
};

class RaycastShape : public PhysicsShape {
public:
  Physics* m_pPhysics;

  // not normalized, length included
  typedef std::vector<Vec4f> RayList;
  RayList m_rays;

  RaycastShape(Physics* phys) : m_pPhysics(phys) {}

  void AddCapsuleRays(float size);

  virtual bool DoesMovementCollide(
      const Mat4f& orientation, const Vec4f& position,
      const Vec4f& velocity, float deltaTime, Vec4f& outVelocity); 
};


} // namespace fd