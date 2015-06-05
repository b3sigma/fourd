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

bool RaycastShape::GenerateImpulse(const Mat4f& orientation, const Vec4f& position,
  const Vec4f& velocity, float deltaTime, Vec4f& outVelocity) {

  Vec4f frameVelocity = velocity * deltaTime;

  bool hitSomething = false;
  Vec4f resultPush(0.0f, 0.0f, 0.0f, 0.0f);
  for(auto ray : m_rays) {
    Vec4f pushedRay(ray + frameVelocity);
    float dist;
    if(m_pPhysics->RayCast(position, pushedRay, &dist)) {
      //Vec4f collideRay = pushedRay
      float rayLen = pushedRay.length();
      resultPush += pushedRay * (dist - rayLen); // push away from ray
      hitSomething = true;
    }
  }
  outVelocity = resultPush;
  return hitSomething;
}

} // namespace fd