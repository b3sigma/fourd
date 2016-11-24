#pragma once

#include <vector>
#include "component.h"
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

// TODO move this to components dir if this pans out
class RaycastShape : public PhysicsShape, public Component {
public:
  Physics* m_pPhysics;

  Mat4f* m_pOwnerOrientation = NULL;
  Vec4f* m_pOwnerPosition = NULL;
  Vec4f* m_pOwnerVelocity = NULL;

  // not normalized, length included
  typedef std::vector<Vec4f> RayList;
  RayList m_rays;
  bool m_oriented;

  RaycastShape(Physics* phys, bool oriented) : m_pPhysics(phys), m_oriented(oriented) {}

  virtual void OnConnected();

  void AddCapsuleRays(float legHeight, float sphereRadius);
  void AddRays(const RayList& rays);

  virtual bool DoesCollide(
      float& deltaTime,
      const Mat4f& orientation, const Vec4f& position,
      Vec4f& safePos, Vec4f& collidePos, Vec4f& collideNormal);

  virtual bool DoesMovementCollide(
      const Mat4f& orientation, const Vec4f& position,
      const Vec4f& velocity, float& deltaTime,
      Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal); 
};


} // namespace fd