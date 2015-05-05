#pragma once

#include "fourmath.h"
#include "component.h"

namespace fd {

  // wow seriously are we writing our own physics lib? lulz
  class RigidBody {
    
  };


  class Physics {
  public:
    // experiment doing mostly public vars to avoid get/set spam

    Vec4f m_groundNormal; //ground plane normal
    float m_groundHeight; 
    Vec4f m_gravity;
    float m_cushion; // floating point cushion

  public:
    Physics() 
      : m_groundNormal(0.0f, 0.0f, 1.0f, 0.0f) // default to z up, cuz fashion
      , m_groundHeight(0.0f)
      , m_gravity(0.0f, 0.0f, -10.0f, 0.0f) // default gravity down
      , m_cushion(0.0001f)
    {
    }

    ~Physics() {}

    void Step(float fDelta);

    // returns true if hit, overwrites distance with amount if so.
    bool RayCast(const Vec4f* position, const Vec4f* direction, float* outDistance);

    bool RayCastGround(const Vec4f* position, const Vec4f* direction, float* outDistance);
    void ClampToGround(Vec4f* position);

    static void TestPhysics();
  };
}; //namespace fd