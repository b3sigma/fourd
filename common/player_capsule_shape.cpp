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

    float velScalar = collisionNormal.dot(velocity);
    if(velScalar < -1.0f) {
      Vec4f negatedNormalVel = velocity * (1.0f / fabs(velScalar));
      outVelocity = outVelocity + negatedNormalVel;
    } else {
      outVelocity = Vec4f(0,0,0,0);
    }
    //outVelocity = Vec4f(0,0,0,0);
    //validPos = position; //hitPos;
    //float frameVelLen = frameVelocity.length();
    //float interpenetrateDist = (hitPos - position).length() - m_radius;


    //Vec4f effectiveVel = hitPos - position;
    //outVelocity = effectiveVel * (1.0f / deltaTime);
    return true;
  }
  return false;
}



} // namespace fd