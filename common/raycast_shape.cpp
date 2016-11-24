#include "fourmath.h"
#include "physics.h"
#include "raycast_shape.h"
#include "tweak.h"

namespace fd {

void RaycastShape::OnConnected() {
  static std::string BDATpos("position");
  static std::string BDATorient("orientation");
  static std::string BDATvelocity("velocity");
      
  if(!m_ownerBus->GetOwnerData(BDATorient, true, &m_pOwnerOrientation)
      || !m_ownerBus->GetOwnerData(BDATpos, true, &m_pOwnerPosition)
      || !m_ownerBus->GetOwnerData(BDATvelocity, true, &m_pOwnerVelocity)) {
    assert(false);
    SelfDestruct(); // huh, won't this cause a crash in physicscomponent? bleh
  }
}

void RaycastShape::AddCapsuleRays(float legHeight, float sphereRadius) {
  m_rays.emplace_back(sphereRadius, 0.0f, 0.0f, 0.0f);
  m_rays.emplace_back(-sphereRadius, 0.0f, 0.0f, 0.0f);
  m_rays.emplace_back(0.0f, sphereRadius, 0.0f, 0.0f);
  m_rays.emplace_back(0.0f, -legHeight, 0.0f, 0.0f);
  m_rays.emplace_back(0.0f, 0.0f, sphereRadius, 0.0f);
  m_rays.emplace_back(0.0f, 0.0f, -sphereRadius, 0.0f);
  m_rays.emplace_back(0.0f, 0.0f, 0.0f, sphereRadius);
  m_rays.emplace_back(0.0f, 0.0f, 0.0f, -sphereRadius);
}

static const float rayEpsilon = 0.00001f; //ugh
void RaycastShape::AddRays(const RaycastShape::RayList& rays) {
  m_rays.reserve(m_rays.size() + rays.size());
  for(auto ray : rays) {
    if(ray.lengthSq() > rayEpsilon) {
      m_rays.emplace_back(ray);
    }
    // don't use zero length rays because problems
  }
}

bool RaycastShape::DoesCollide(
    float& deltaTime, const Mat4f& orientation, const Vec4f& position,
    Vec4f& safePos, Vec4f& collidePos, Vec4f& collideNormal) {

  Vec4f bestHitPos;
  Vec4f bestHitNormal;
  Vec4f bestHitRay;
  float closestHitSq = FLT_MAX;
  bool hitSomething = false;
  for(Vec4f ray : m_rays) {
    if(m_oriented && m_pOwnerOrientation) {
      ray = m_pOwnerOrientation->transform(ray);
    }
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
  for(Vec4f ray : m_rays) {
    if(m_oriented && m_pOwnerOrientation) {
      ray = m_pOwnerOrientation->transform(ray);
    }

    Vec4f pushedRay(ray + frameVelocity);
    if(pushedRay.lengthSq() <= rayEpsilon)
      continue;
    float dist;
    if(m_pPhysics->RayCast(position, pushedRay, &dist, &hitNormal)) {
      Vec4f collideRay = pushedRay.normalized() * dist;
      Vec4f correctedVel = collideRay - ray;
      if(correctedVel.lengthSq() > rayEpsilon && correctedVel.lengthSq() > frameVelocity.lengthSq()) {
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

  // the idea is to block the player from sliding around in the w direction, which is disorienting without knowledge of your own physics volume
  static TweakVariable lockTo3Space("game.playerLock3Space", false);
  if(lockTo3Space.AsBool()) {
    Vec4f cameraSpaceVelocity =  frameVelocity;

    static TweakVariable tweakInvertedCamera("game.playerLock3Inverted", false);
    if(tweakInvertedCamera.AsBool()) {
      cameraSpaceVelocity = m_pOwnerOrientation->inverse().transform(cameraSpaceVelocity);
    } else {
      cameraSpaceVelocity = m_pOwnerOrientation->transform(cameraSpaceVelocity);
    }

    cameraSpaceVelocity.w = 0.0f;
    
    if(tweakInvertedCamera.AsBool()) {
      cameraSpaceVelocity = m_pOwnerOrientation->transform(cameraSpaceVelocity);
    } else {
      cameraSpaceVelocity = m_pOwnerOrientation->inverse().transform(cameraSpaceVelocity);
    }

    frameVelocity = cameraSpaceVelocity;
  }

  outVelocity = frameVelocity * (1.0f / deltaTime);
  return hitSomething;
}

} // namespace fd