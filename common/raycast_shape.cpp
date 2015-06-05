#include "fourmath.h"
#include "physics.h"
#include "raycast_shape.h"

namespace fd {

void RaycastShape::AddCapsuleRays(float size) {
  m_rays.emplace_back(size, 0.0f, 0.0f, 0.0f);
  m_rays.emplace_back(-size, 0.0f, 0.0f, 0.0f);
  m_rays.emplace_back(0.0f, size, 0.0f, 0.0f);
  m_rays.emplace_back(0.0f, -size, 0.0f, 0.0f);
  m_rays.emplace_back(0.0f, 0.0f, size, 0.0f);
  m_rays.emplace_back(0.0f, 0.0f, -size, 0.0f);
  m_rays.emplace_back(0.0f, 0.0f, 0.0f, size);
  m_rays.emplace_back(0.0f, 0.0f, 0.0f, -size);
}

bool RaycastShape::DoesMovementCollide(const Mat4f& orientation,
    const Vec4f& position, const Vec4f& velocity, float deltaTime,
    Vec4f& outVelocity) {

  outVelocity = velocity;
  Vec4f frameVelocity = outVelocity * deltaTime;

  bool hitSomething = false;
  for(auto ray : m_rays) {
    Vec4f pushedRay(ray + frameVelocity);
    float dist;
    if(m_pPhysics->RayCast(position, pushedRay, &dist)) {
      Vec4f collideRay = pushedRay.normalized() * dist;
      Vec4f correctedVel = collideRay - ray;
      if(correctedVel.lengthSq() > frameVelocity.lengthSq()) {
        frameVelocity = correctedVel.normalized() * frameVelocity.length();
      } else {
        frameVelocity = correctedVel;
      }
      hitSomething = true;
    }
  }
  outVelocity = frameVelocity * (1.0f / deltaTime);
  return hitSomething;
}

} // namespace fd