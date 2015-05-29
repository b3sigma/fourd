#include "physics_help.h"

namespace fd {

bool PhysicsHelp::RayToPlane(
    const Vec4f& start, const Vec4f& ray,
    const Vec4f& planeNormal, float planeOffset,
    Vec4f* outCollisionPoint, float* outDistance) {
  Vec4f localStart(start);
  localStart -= (planeNormal * planeOffset);
  Vec4f localEnd(localStart);
  localEnd += ray;

  float startDotPlane = planeNormal.dot(localStart);
  float endDotPlane = planeNormal.dot(localEnd);
  if(startDotPlane * endDotPlane > 0.0f) {
    return false; // both on same side, no collision
  } else {
    if(outCollisionPoint || outDistance) {
      float percentToCollision = abs(startDotPlane)
          / (abs(startDotPlane) + abs(endDotPlane));
      if(outCollisionPoint) {
        *outCollisionPoint = start;
        *outCollisionPoint += (ray * percentToCollision);
      }
      if(outDistance) {
        *outDistance = ray.length() * percentToCollision;
      }
    }
    return true;
  }
}
    
bool PhysicsHelp::RayToAlignedBox(
    const Vec4f& min, const Vec4f& max,
    const Vec4f& start, const Vec4f& ray,
    float* outDist, Vec4f* outPoint) {
  // ugh we are going to want a normal also soon
  Vec4f end(start);
  end += ray;
  bool startWithinBox = WithinBox(min, max, start);
  bool endWithinBox = WithinBox(min, max, end);
  if (startWithinBox && endWithinBox) return false; // uh?

  float smallestDist = 99999999.0f;
  Vec4f bestPoint;
  bool foundPoint = false;
      
  float borderThreshold = 0.0001f;
  const Vec4f toleranceVect(
      borderThreshold, borderThreshold, borderThreshold, borderThreshold);
  Vec4f toleranceMin(min - toleranceVect);
  Vec4f toleranceMax(max + toleranceVect);

  for(int whichPlane = 0; whichPlane < 8; ++whichPlane) {
    Vec4f plane(0.0f, 0.0f, 0.0f, 0.0f);
    plane[whichPlane % 4] = (whichPlane < 4) ? 1.0f : -1.0f;
    float planeOffset;
    if(whichPlane < 4) {
      planeOffset = max[whichPlane];
    } else {
      planeOffset = -min[whichPlane % 4];
    }
    Vec4f collisionPoint;
    float collisionDist;
    if(!RayToPlane(start, ray, plane, planeOffset,
        &collisionPoint, &collisionDist)) {
      continue;
    }
    if(WithinBox(toleranceMin, toleranceMax, collisionPoint)) {
      if(collisionDist < smallestDist) {
        smallestDist = collisionDist;
        bestPoint = collisionPoint;
        foundPoint = true;
      }
    }
  }

  if(foundPoint) {
    if(outDist) {
      *outDist = smallestDist;
    }
    if(outPoint) {
      *outPoint = bestPoint;
    }
    return true;
  } else {
    return false;
  }
}


}; // namespace fd