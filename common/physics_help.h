#pragma once

#include "fourmath.h"
#include "quaxol.h"

namespace fd {

  class PhysicsHelp {
  public:
    template <typename TVec>
    inline static bool WithinBox(const TVec& min, const TVec& max, const TVec& point) {
      const int numCoords = sizeof(point) / sizeof(point[0]);
      for(int coord = 0; coord < numCoords; coord++) {
        if(point[coord] < min[coord] || point[coord] > max[coord])
          return false;
      }
      return true;
    }

    static bool RayToPlane(
        const Vec4f& start, const Vec4f& ray,
        const Vec4f& planeNormal, float planeOffset,
        Vec4f* outCollisionPoint, float* outDistance) {
      Vec4f localStart(start);
      localStart -= (planeNormal * planeOffset);
      Vec4f localEnd(localStart);
      localEnd += ray;

      float startDotPlane = planeNormal.dot(localStart);
      float endDotPlane = planeNormal.dot(localEnd);
      if(startDotPlane * endDotPlane >= 0.0f) {
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
    
    inline static bool RayToAlignedBox(
        const Vec4f& min, const Vec4f& max,
        const Vec4f& start, const Vec4f& ray,
        float* outDist, Vec4f* outPoint) {
      // ugh we are going to want a normal also soon
      Vec4f end(start);
      end += ray;
      bool startWithinBox = WithinBox(min, max, start);
      bool endWithinBox = WithinBox(min, start, end);
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

    inline static bool RayToQuaxol(
        const QuaxolSpec& pos,
        const Vec4f& start, const Vec4f& ray,
        float* outDist, Vec4f* outPoint) {
      // probably going the wrong way in terms of efficiency... 
      Vec4f minBox((float)pos.x, (float)pos.y, (float)pos.z, (float)pos.w);
      Vec4f maxBox((float)(pos.x + 1), (float)(pos.y + 1), (float)(pos.z + 1), (float)(pos.w + 1));
      return RayToAlignedBox(minBox, maxBox, start, ray, outDist, outPoint);
    }

    //returns true if any part was clipped
    inline static bool ClipRayToBox(const Vec4f& min, const Vec4f& max,
        const Vec4f& start, const Vec4f& ray,
        Vec4f* clippedStart, Vec4f* clippedEnd) {
      if(WithinBox(min, max, start)) 
        return true;

      Vec4f localMax(max);
      localMax -= min;
      // localmin = 0
      Vec4f localStart(start);
      localStart -= min;
      Vec4f localEnd(start + ray);
      localEnd -= min;

      return false;
    }
  };

}; //namespace fd