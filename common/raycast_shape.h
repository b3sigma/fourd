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
  virtual bool GenerateImpulse(const Mat4f& orientation, const Vec4f& position,
    const Vec4f& frameVelocity, Vec4f& outImpulse) { return false; }
};

class RaycastShape : public PhysicsShape {
public:
  Physics* m_pPhysics;

  // not normalized, length included
  typedef std::vector<Vec4f> RayList;
  RayList m_rays;

  RaycastShape(Physics* phys) : m_pPhysics(phys) {}

  void AddCapsuleRays(float size) {
    m_rays.emplace_back(size, 0.0f, 0.0f, 0.0f);
    m_rays.emplace_back(-size, 0.0f, 0.0f, 0.0f);
    m_rays.emplace_back(0.0f, size, 0.0f, 0.0f);
    m_rays.emplace_back(0.0f, -size, 0.0f, 0.0f);
    m_rays.emplace_back(0.0f, 0.0f, size, 0.0f);
    m_rays.emplace_back(0.0f, 0.0f, -size, 0.0f);
    m_rays.emplace_back(0.0f, 0.0f, 0.0f, size);
    m_rays.emplace_back(0.0f, 0.0f, 0.0f, -size);
  }

  virtual bool GenerateImpulse(const Mat4f& orientation, const Vec4f& position,
    const Vec4f& frameVelocity, Vec4f& outImpulse) {

    bool hitSomething = false;
    Vec4f resultPush(0.0f, 0.0f, 0.0f, 0.0f);
    for(auto ray : m_rays) {
      Vec4f pushedRay(ray + frameVelocity);
      float dist;
      if(m_pPhysics->RayCast(position, pushedRay, &dist)) {
        float rayLen = pushedRay.length();
        resultPush += pushedRay * (dist - rayLen); // push away from ray
        hitSomething = true;
      }
    }
    outImpulse = resultPush;
    return hitSomething;
  }

};


} // namespace fd