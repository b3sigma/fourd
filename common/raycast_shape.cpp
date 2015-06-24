#include "fourmath.h"
#include "physics.h"
#include "raycast_shape.h"

namespace fd {

void RaycastShape::AddCapsuleRays(float size) {
  //m_rays.emplace_back(size, 0.0f, 0.0f, 0.0f);
  //m_rays.emplace_back(-size, 0.0f, 0.0f, 0.0f);
  //m_rays.emplace_back(0.0f, size, 0.0f, 0.0f);
  m_rays.emplace_back(0.0f, -size, 0.0f, 0.0f);
  //m_rays.emplace_back(0.0f, 0.0f, size, 0.0f);
  //m_rays.emplace_back(0.0f, 0.0f, -size, 0.0f);
  //m_rays.emplace_back(0.0f, 0.0f, 0.0f, size);
  //m_rays.emplace_back(0.0f, 0.0f, 0.0f, -size);
}

bool RaycastShape::DoesCollide(
    float& deltaTime, const Mat4f& orientation, const Vec4f& position,
    Vec4f& safePos, Vec4f& collidePos, Vec4f& collideNormal) {

  Vec4f bestHitPos;
  Vec4f bestHitNormal;
  Vec4f bestHitRay;
  float closestHitSq = FLT_MAX;
  bool hitSomething = false;
  for(auto ray : m_rays) {
    float dist;
    Vec4f localHitNormal;
    if(m_pPhysics->RayCast(position, ray, &dist, &localHitNormal)) {
      Vec4f collidePos = ray.normalized() * dist;
      float hitDistanceSq = (collidePos - ray).lengthSq();
      if(hitDistanceSq < closestHitSq) {
        closestHitSq = hitDistanceSq;
        hitSomething = true;
        bestHitNormal = localHitNormal;
        bestHitPos = collidePos;
        bestHitRay = ray;
      }
    }
  }

  if(hitSomething) {
    collidePos = bestHitPos;
    collideNormal = bestHitNormal;
    safePos = collidePos + bestHitRay;
  }

  return hitSomething;
}

bool RaycastShape::DoesMovementCollide(const Mat4f& orientation,
    const Vec4f& position, const Vec4f& velocity, float& deltaTime,
    Vec4f& validPos, Vec4f& outVelocity, Vec4f& collisionNormal) {

  outVelocity = velocity;
  Vec4f frameVelocity = outVelocity * deltaTime;

  Vec4f hitNormal;
  float closestHit = FLT_MAX;
  bool hitSomething = false;
  for(auto ray : m_rays) {
    Vec4f pushedRay(ray + frameVelocity);
    float dist;
    if(m_pPhysics->RayCast(position, pushedRay, &dist, &hitNormal)) {
      Vec4f collideRay = pushedRay.normalized() * dist;
      Vec4f correctedVel = collideRay - ray;
      if(correctedVel.lengthSq() > frameVelocity.lengthSq()) {
        frameVelocity = correctedVel.normalized() * frameVelocity.length();
      } else {
        frameVelocity = correctedVel;
      }
      if(dist < closestHit) {
        closestHit = dist;
        collisionNormal = hitNormal;
        validPos = (ray.normalized() * dist) + position;
      }
      hitSomething = true;
    }
  }
  outVelocity = frameVelocity * (1.0f / deltaTime);
  return hitSomething;
}

} // namespace fd