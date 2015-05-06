#pragma once

#include "fourmath.h"
#include "chunkloader.h"
#include "component.h"
#include "quaxol.h"

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

    // this is the wrong abstraction, should have a wrapper that handles
    // multiple chunks. Will incorporate later.
    QuaxolChunk* m_chunk;

  public:
    Physics() 
      : m_groundNormal(0.0f, 0.0f, 1.0f, 0.0f) // default to z up, cuz fashion
      , m_groundHeight(0.0f)
      , m_gravity(0.0f, 0.0f, -10.0f, 0.0f) // default gravity down
      , m_cushion(0.0001f)
      , m_chunk(NULL)
    {
    }

    ~Physics() {}
    
    void AddChunk(QuaxolChunk* pChunk) { m_chunk = pChunk; }

    void Step(float fDelta);

    // returns true if hit, overwrites distance with amount if so.
    bool RayCast(const Vec4f* position, const Vec4f* direction, float* outDistance);

    bool RayCastGround(const Vec4f* position, const Vec4f* direction, float* outDistance);
    void ClampToGround(Vec4f* position);

    bool RayCastChunk(const QuaxolChunk& chunk,
        const Vec4f& position, const Vec4f& ray, float* outDistance);

    void LineDraw2D(const Vec4f& position, const Vec4f& ray, float* outDist,
        DelegateN<bool, const QuaxolChunk*, int, int, int, int> callback);
    void LineDraw4D(const Vec4f& position, const Vec4f& ray, float* outDist,
        DelegateN<bool, const QuaxolChunk*, int, int, int, int> callback);

    static void TestPhysics();
  protected:
    static TVecQuaxol s_testQuaxols;
    static bool TestPhysicsCallback(const QuaxolChunk* chunk, int x, int y, int z, int w);
  };
}; //namespace fd