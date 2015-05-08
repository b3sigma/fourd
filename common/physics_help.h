#pragma once

#include "fourmath.h"

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

    ////returns true if any part was clipped
    //inline static bool ClipRayToBox(const Vec4f& min, const Vec4f& max,
    //    const Vec4f& start, const Vec4f& end,
    //    Vec4f* clippedStart, Vec4f* clippedEnd) {
    //  if(WithinBox(min, max, start)) 
    //    return true;
    //  return false;
    //}
  };

}; //namespace fd