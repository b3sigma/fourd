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
        Vec4f* outCollisionPoint, float* outDistance);
    
    static bool RayToAlignedBox(
        const Vec4f& min, const Vec4f& max,
        const Vec4f& start, const Vec4f& ray,
        float* outDist, Vec4f* outPoint, Vec4f* outNormal);
    static bool RayToAlignedBox(
        const Vec4f& min, const Vec4f& max,
        const Vec4f& start, const Vec4f& ray,
        float* outDist, Vec4f* outPoint);


    inline static bool RayToQuaxol(
        const QuaxolSpec& pos,
        const Vec4f& start, const Vec4f& ray,
        float* outDist, Vec4f* outPoint, Vec4f* outNormal) {
      // probably going the wrong way in terms of efficiency... 
      Vec4f minBox((float)pos.x, (float)pos.y, (float)pos.z, (float)pos.w);
      Vec4f maxBox((float)(pos.x + 1), (float)(pos.y + 1), (float)(pos.z + 1), (float)(pos.w + 1));
      return RayToAlignedBox(minBox, maxBox, start, ray,
          outDist, outPoint, outNormal);
    }

    ////returns true if any part was clipped
    //inline static bool ClipRayToBox(const Vec4f& min, const Vec4f& max,
    //    const Vec4f& start, const Vec4f& ray,
    //    Vec4f* clippedStart, Vec4f* clippedEnd) {
    //  if(WithinBox(min, max, start)) 
    //    return true;

    //  Vec4f localMax(max);
    //  localMax -= min;
    //  // localmin = 0
    //  Vec4f localStart(start);
    //  localStart -= min;
    //  Vec4f localEnd(start + ray);
    //  localEnd -= min;

    //  return false;
    //}
  };

}; //namespace fd