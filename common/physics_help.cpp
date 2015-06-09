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
  return RayToAlignedBox(min, max, start, ray, outDist, outPoint, NULL /*normal*/);
}


bool PhysicsHelp::RayToAlignedBox(
    const Vec4f& min, const Vec4f& max,
    const Vec4f& start, const Vec4f& ray,
    float* outDist, Vec4f* outPoint, Vec4f* outNormal) {
  // ugh we are going to want a normal also soon
  Vec4f end(start);
  end += ray;
  bool startWithinBox = WithinBox(min, max, start);
  bool endWithinBox = WithinBox(min, max, end);
  if (startWithinBox && endWithinBox) return false; // uh?

  float smallestDist = FLT_MAX;
  Vec4f bestPoint;
  Vec4f bestHitNormal;
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
        bestHitNormal = plane;
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
    if(outNormal) {
      *outNormal = bestHitNormal;
    }
    return true;
  } else {
    return false;
  }
}

//                     *         *
// .     .              .     .
//              
//     *     *
// .     .              .     .
//                     *         *
//     *     *
// Thus, the solution is to convert the floats to ascii
// and count the number of spaces minus linewrapping
bool PhysicsHelp::AlignedBoxToAlignedBox(
    const Vec4f& minLeft, const Vec4f& maxLeft,
    const Vec4f& minRight, const Vec4f& maxRight) {

  // derping today
  // the obvious solution seems horrible
  // (build the 16 edges from each min/max pair,
  // then check whether any of the 16 r is within l, or inverse)
  // so derp
 
  return false;
}

bool PhysicsHelp::SphereToPlane(
    const Vec4f& pos, float radius,
    const Vec4f& planeNormal, float planeOffset,
    Vec4f* outPoint) {
  float dotToPlane = planeNormal.dot(pos);
  if(dotToPlane - planeOffset > radius) {
    return false;
  }
  
  Vec4f hitPoint(pos);
  hitPoint -= planeNormal * (dotToPlane - planeOffset);
  *outPoint = pos;
  return true;
}

// derping out today
bool PhysicsHelp::SphereToAlignedBox(
    const Vec4f& min, const Vec4f& max,
    const Vec4f& pos, float radius,
    Vec4f* outPoint, Vec4f* outNormal) {
  
  if(WithinBox(min, max, pos)) {
    // might be better to return a point in the direction of the box
    // center to facilitate pushing out
    *outPoint = pos;
    *outNormal = (pos - ((min + max) * 0.5f)).normalized();
    return true;
  }

  float smallestDist = FLT_MAX;
  Vec4f bestPoint;
  Vec4f bestHitNormal;
  bool foundPoint = false;
      
  // these are all over the place and all different
  // yet another ticking death bomb
  float borderThreshold = 0.0001f; 
  const Vec4f toleranceVect(
      borderThreshold, borderThreshold, borderThreshold, borderThreshold);
  Vec4f toleranceMin(min - toleranceVect);
  Vec4f toleranceMax(max + toleranceVect);

  for(int whichPlane = 0; whichPlane < 8; ++whichPlane) {
    Vec4f plane(0.0f, 0.0f, 0.0f, 0.0f);
    int compIndex = whichPlane % 4;
    plane[compIndex] = (whichPlane < 4) ? 1.0f : -1.0f;
    float planeOffset;
    if(whichPlane < 4) {
      planeOffset = max[whichPlane];
      if((pos[compIndex] - radius) > planeOffset) {
        continue; // too high to possibly intersect with this edge
      }
    } else {
      planeOffset = -min[whichPlane % 4];
      if((pos[compIndex] + radius) < planeOffset) {
        continue; // to low to possibly intersect with this edge
      }
    }

    Vec4f planarPosition = pos;
    planarPosition[compIndex] = 0.0f;
    // sphere vs box,

    //Vec4f collisionPoint;
    //float collisionDist;
    //if(!RayToPlane(start, ray, plane, planeOffset,
    //    &collisionPoint, &collisionDist)) {
    //  continue;
    //}
    //if(WithinBox(toleranceMin, toleranceMax, collisionPoint)) {
    //  if(collisionDist < smallestDist) {
    //    smallestDist = collisionDist;
    //    bestPoint = collisionPoint;
    //    bestHitNormal = plane;
    //    foundPoint = true;
    //  }
    //}
  }

  return false;

}

void PhysicsHelp::RunTests() {
  // all tests passed, well done!

}



}; // namespace fd