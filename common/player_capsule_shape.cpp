#include "player_capsule_shape.h"
#include "physics.h"
#include "physics_help.h"

namespace fd {

 
bool PlayerCapsuleShape::DoesMovementCollide(
    const Mat4f& orientation, const Vec4f& position,
    const Vec4f& velocity, float deltaTime,
    Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal) {
  
  Vec4f radiusVec(m_radius, m_radius, m_radius, m_radius);
  Vec4f min = position - radiusVec;
  Vec4f max = position + radiusVec;

  outVelocity = velocity;
  Vec4f frameVelocity = outVelocity * deltaTime;

  // the approach is to test collision with the result position
  // if the collides, then use the hitposition to set a farther back position
  Vec4f testPosition = position + frameVelocity;
  Vec4f hitPos;
  if(m_pPhysics->SphereCollide(testPosition, m_radius,
      &hitPos, &collisionNormal)) {
    validPos = hitPos;
    Vec4f effectiveVel = hitPos - position;
    outVelocity = effectiveVel * (1.0f / deltaTime);
  }
  return false;
}



} // namespace fd