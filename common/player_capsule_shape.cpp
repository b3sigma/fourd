#include "player_capsule_shape.h"
#include "physics.h"
#include "physics_help.h"

namespace fd {
  
bool PlayerCapsuleShape::ApplyMovement(
    float& deltaTime, Vec4f& velocity,
    Mat4f& orientation, Vec4f& position,
    bool& hadGroundCollision) {

  bool hitSomething = false;
  bool madeProgress = false;

  Vec4f bestPosition = position;
  Vec4f attempVelocity = velocity;

  float remainingDeltaTime = deltaTime;
  const int maxSteps = 4;
  for(int s = 0; s < maxSteps; s++) {
  
    Vec4f deltaPosition = attempVelocity * remainingDeltaTime;
    Vec4f testPosition = bestPosition + deltaPosition;

    // this has ended up as any position within the intersection of the sphere
    // and the other shape
    Vec4f collisionPos;
    // this ends up being nearly anything it seems
    Vec4f collisionNormal;
    if(m_pPhysics->SphereCollide(
        testPosition, m_radius, &collisionPos, &collisionNormal)) {

      // cheat and override
      collisionNormal = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
      float colDotVel = attempVelocity.dot(collisionNormal);
      Vec4f removeVel = collisionNormal * (-1.0f * colDotVel);
      attempVelocity += removeVel;

      //remainingDeltaTime *= 0.5f;
      //bestPosition = collisionPos;
      hitSomething = true;
      continue;
    } else {
      madeProgress = true;
      bestPosition = testPosition;
      break;
    }
  }

  if(madeProgress) {
    velocity = attempVelocity;
    position = bestPosition;
  }
  
  return hitSomething;
}


bool PlayerCapsuleShape::DoesCollide(
    float& deltaTime, const Mat4f& orientation, const Vec4f& position,
    Vec4f& safePos, Vec4f& collidePos, Vec4f& collideNormal) {
  
  if(m_pPhysics->SphereCollide(position, m_radius,
      &collidePos, &collideNormal)) {
    Vec4f collideDir = position - collidePos;
    safePos = collidePos + (collideDir.normalized() * m_radius);

    //if(m_pPhysics->SphereCollide(safePos, m_radius,
    //    NULL /*pos*/, NULL /*normal*/)) {
    //  Vec4f garbage = safePos;

    //  return true;
    //}


    return true;
  }
  return false;
}
 
bool PlayerCapsuleShape::DoesMovementCollide(
    const Mat4f& orientation, const Vec4f& position,
    const Vec4f& velocity, float& deltaTime,
    Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal) {
  
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
      //outVelocity = outVelocity * 0.1f;
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