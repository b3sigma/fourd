#include <float.h>
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
      float percentToCollision = fabs(startDotPlane)
          / (fabs(startDotPlane) + fabs(endDotPlane));
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
  if(outPoint) {
    *outPoint = hitPoint;
  }
  return true;
}

// expand everything else by radius, treat sphere like a point
bool PhysicsHelp::SphereToAlignedBoxMinkowski(
    const Vec4f& min, const Vec4f& max,
    const Vec4f& pos, float radius,
    Vec4f* outPoint, Vec4f* outNormal) {

  Vec4f radiusVec(radius, radius, radius, radius);

  if(!WithinBox(min - radiusVec, max + radiusVec, pos)) {
    return false;
  }

  bool hadCollision = false;
  bool withinNonMinkowski = false;

  if(WithinBox(pos, min, max)) {
    hadCollision = true;
    withinNonMinkowski = true;
  }

  //// so enlarge in each direction to do easy minkowski cases
  //for(int c = 0; c < 4; c++) {
  //  Vec4f kowMin(min);
  //  Vec4f kowMax(max);
  //  kowMin[c] -= radius;
  //  kowMax[c] += radius;

  //  if(WithinBox(kowMin, kowMax, pos)) {
  //    hadCollision = true;
  //    break;
  //  }
  //}

  if(!hadCollision) {
    // not hitting above means we are between box and minkowski box
    // maybe in general we can classify a point as to how many dimensions
    //  it isn't within the unenlarged bounds of, and then get distances
    //  per component.
    // for each component outside, accum dist^2, then sqrt, then < radius

    float accumDistSq = 0.0f;
    for(int c = 0; c < 4; c++) {
      if(pos[c] < min[c]) {
        float diff = pos[c] - min[c];
        accumDistSq += diff * diff;
      } else if(pos[c] > max[c]) {
        float diff = pos[c] - max[c];
        accumDistSq += diff * diff;
      }
    }

    if(accumDistSq < (radius * radius)) {
      hadCollision = true;
    }
  }

  if(!hadCollision) {
    // still no collision means no collision
    return false;
  }

  // project the sphere center on the box as a proxy for collision point
  // if the shere is within the tesseract, the point will just be the center
  Vec4f projectedPoint = pos;
  Vec4f projectedNormal(0.0f, 0.0f, 0.0f, 0.0f);
  for(int w = 0; w < 4; ++w) {
    if(projectedPoint[w] < min[w]) {
      projectedPoint[w] = min[w];
      projectedNormal[w] = -1.0f;
    } else if(projectedPoint[w] > max[w]) {
      projectedPoint[w] = max[w];
      projectedNormal[w] = 1.0f;
    }
  }

  if(outNormal) {
    if(projectedNormal.lengthSq() > 0.0f) {
      *outNormal = projectedNormal.normalized();
    } else { // within the tesseract, just use the delta?
      Vec4f posDelta = ((min + max) * 0.5f) - pos;
      if(posDelta.lengthSq() > 0.0f) {
        *outNormal = posDelta.normalized();
      } else {
        // right in the middle of box, totally undefined so return up
        // person who wanted to redefine up, but ran into a bug <1% of the time,
        // and just found this bit of code, fuck you :)
        *outNormal = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
      }
    }
  }

  if(outPoint) {
    *outPoint = projectedPoint;
  }


  //// now to guess at collision point, find closest wall
  //// for a hit, this will be 0-3 for min, 4-8 for max
  //int closestWall;
  //float smallestDist = FLT_MAX;
  //for(int w = 0; w < 4; ++w) {
  //  float distToMin = fabs(pos[w] - min[w]);
  //  if(distToMin < smallestDist) {
  //    closestWall = w;
  //    smallestDist = distToMin;
  //  }
  //  float distToMax = fabs(pos[w] - max[w]);
  //  if(distToMax < smallestDist) {
  //    closestWall = w + 4;
  //    smallestDist = distToMax;
  //  }
  //}

  //if(outNormal) {
  //  Vec4f hitNormal(0,0,0,0);
  //  hitNormal[closestWall % 4] = (closestWall > 4) ? 1.0f : -1.0f;
  //  *outNormal = hitNormal;
  //}

  //if(outPoint) {
  //  // try the really stupid thing for a moment
  //  //if(withinNonMinkowski) {
  //    Vec4f returnPos(pos);
  //    int coord = closestWall % 4;
  //    returnPos[coord] = (closestWall > 4) ? max[coord] : min[coord];
  //    *outPoint = returnPos;
  //  //} else {
  //  //  // um, shrink the point down inversely to how the box was enlarged
  //  //  // then do ray between start point and shrunk point to associated plane?
  //  //  // nope, that's wrong, but it's not well defined anyway, so maybe it's fine
  //  //  Vec4f boxMid = (min + max) * 0.5f;
  //  //  Vec4f localCoord = pos - boxMid;
  //  //  float boxRadius = ((max + radiusVec) - boxMid).length();
  //  //  float shrinkRatio = boxRadius / (radius + boxRadius);
  //  //  Vec4f shrunkLocalCoord(localCoord);
  //  //  shrunkLocalCoord *= shrinkRatio;
  //  //  Vec4f rayInward = shrunkLocalCoord - localCoord;
  //  //  float shrinkEpsilon = 0.001f; // actually makes it slinkly less shrunk
  //  //  localCoord *= (1.0f + shrinkEpsilon);

  //  //  if(!PhysicsHelp::RayToAlignedBox(min, max,
  //  //      localCoord + boxMid, rayInward,
  //  //      NULL /*dist*/, outPoint)) {
  //  //    assert(false); // huh, that should have worked
  //  //    //whatever, pick a point time
  //  //    *outPoint = shrunkLocalCoord + boxMid;
  //  //  }
  //  //}
  //}

  return true;
}

bool PhysicsHelp::SphereToAlignedBox(
    const Vec4f& min, const Vec4f& max,
    const Vec4f& pos, float radius,
    Vec4f* outPoint, Vec4f* outNormal) {

  if(WithinBox(min, max, pos)) {
    // might be better to return a point in the direction of the box
    // center to facilitate pushing out
    if(outPoint)
      *outPoint = pos;
    if(outNormal)
      *outNormal = (pos - ((min + max) * 0.5f)).normalized();
    return true;
  }

  // kinda broad phase?
  Vec4f boxMid = (min + max) * 0.5f;
  float boxRadius = (max - min).length() * 0.5f;
  float distSpehereToBoxMid = (boxMid - pos).length();
  if(distSpehereToBoxMid - boxRadius - radius > 0.0f)
    return false;

  float smallestDistSq = FLT_MAX;
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

  // seems like we should be able to cut down the number of plane checks to
  // 4(dim), but this is fine for now
  for(int whichPlane = 0; whichPlane < 8; ++whichPlane) {
    Vec4f plane(0.0f, 0.0f, 0.0f, 0.0f);
    int compIndex = whichPlane % 4;
    plane[compIndex] = (whichPlane < 4) ? 1.0f : -1.0f;
    float planeOffset;
    if(whichPlane < 4) {
      planeOffset = max[compIndex];
      if((pos[compIndex] - radius) > planeOffset) {
        continue; // too high to possibly intersect with this edge
      }
    } else {
      planeOffset = min[compIndex];
      if((pos[compIndex] + radius) < planeOffset) {
        continue; // to low to possibly intersect with this edge
      }
    }

    Vec4f hitPos;
    if(SphereToPlane(pos, radius, plane, planeOffset, &hitPos)) {
      Vec4f planeOrigin(boxMid);
      float planeOriginToCornerDist = 0.0f;
      if(whichPlane < 4) {
        planeOrigin[compIndex] = max[compIndex];
        planeOriginToCornerDist = (planeOrigin - max).length();
      } else {
        planeOrigin[compIndex] = min[compIndex];
        planeOriginToCornerDist = (planeOrigin - min).length();
      }

      //// find the closest point on the sphere (or circle in 3d)
      //// in the plane of intersection
      //// then we will do the box test against just that point
      //Vec4f closes

      //
      // bool validHit = true;
      //for(int c = 0; c < 4; c++) {
      //  if(c == compIndex) continue;

      //  if(hitPos[c] < min[c] || hitPos[c] > max[c]) {
      //    validHit = false;
      //    break;
      //  }
      //}

      //if(!validHit)


      float originToHitDist = (hitPos - planeOrigin).length();
      float distToBox = originToHitDist - radius - planeOriginToCornerDist;
      if(distToBox > 0.0f) {
        continue;
      }

      //Vec4f boxHitPos =

      float collisionDistSq = (pos - hitPos).lengthSq();
      if(collisionDistSq < smallestDistSq) {
        smallestDistSq = collisionDistSq;
        bestPoint = hitPos;
        bestHitNormal = plane;
        foundPoint = true;
      }

    }
  }

  if(foundPoint) {
    if(outPoint)
      *outPoint = bestPoint;
    if(outNormal)
      *outNormal = bestHitNormal;
  }

  return foundPoint;
}

void PhysicsHelp::RunTests() {
  Vec4f pos(1.5f, 1.5f, 5.0f, 1.5f);
  float radius = 10.0f;
  Vec4f planeNormal(0.0f, 0.0f, 1.0f, 0.0f);
  float planeOffset = 0.5f;

  Vec4f hitPoint;
  assert(true == SphereToPlane(pos, radius, planeNormal, planeOffset, &hitPoint));

  pos = Vec4f(1.5f, 1.5f, 20.0f, 1.5f);
  assert(false == SphereToPlane(pos, radius, planeNormal, planeOffset, &hitPoint));

  Vec4f min(1.0f, 1.0f, 1.0f, 1.0f);
  Vec4f max(2.0f, 2.0f, 2.0f, 2.0f);
  radius = 1.5f;
  pos = Vec4f(2.5f, 2.5f, 2.5f, 2.5f);
  Vec4f hitNormal;
  assert(true == SphereToAlignedBox(min, max, pos, radius, &hitPoint, &hitNormal));
  assert(true == SphereToAlignedBoxMinkowski(
      min, max, pos, radius, &hitPoint, &hitNormal));

  pos = Vec4f(2.5f, 2.5f, 20.5f, 2.5f);
  assert(false == SphereToAlignedBox(min, max, pos, radius, &hitPoint, &hitNormal));
  assert(false == SphereToAlignedBoxMinkowski(
      min, max, pos, radius, &hitPoint, &hitNormal));

  min.set(1.0f, 1.0f, 1.0f, 0.0f);
  max.set(2.0f, 2.0f, 2.0f, 0.0f);
  pos = Vec4f(2.5f, 2.0f, 2.5f, 0.0f);
  radius = 0.70f;
  //assert(false == SphereToAlignedBox(min, max, pos, radius, &hitPoint, &hitNormal));
  assert(false == SphereToAlignedBoxMinkowski(
      min, max, pos, radius, &hitPoint, &hitNormal));

  min.set(1.0f, 1.0f, 1.0f, 1.0f);
  max.set(2.0f, 2.0f, 2.0f, 2.0f);
  pos = Vec4f(2.5f, 2.0f, 2.5f, 2.0f);
  radius = 0.70f;
  //assert(false == SphereToAlignedBox(min, max, pos, radius, &hitPoint, &hitNormal));
  assert(false == SphereToAlignedBoxMinkowski(
      min, max, pos, radius, &hitPoint, &hitNormal));
}



}; // namespace fd
