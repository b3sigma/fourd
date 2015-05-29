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
    QuaxolChunk* m_chunk; //not owned

  public:
    Physics() 
      : m_groundNormal(0.0f, 0.0f, 1.0f, 0.0f) // default to z up, cuz fashion
      , m_groundHeight(0.0f)
      , m_gravity(0.0f, 0.0f, -50.0f, 0.0f) // default gravity down
      , m_cushion(0.0001f)
      , m_chunk(NULL)
    {
    }

    ~Physics() {}
    
    void AddChunk(QuaxolChunk* pChunk) { m_chunk = pChunk; }

    void Step(float fDelta);

    // returns true if hit, overwrites distance with amount if so.
    bool RayCast(const Vec4f& position, const Vec4f& ray, float* outDistance);
    bool RayCastToOpenQuaxol(const Vec4f& position, const Vec4f& ray,
        QuaxolSpec* outOpenBlock, Vec4f* outPos);
    bool RayCastToPresentQuaxol(const Vec4f& position, const Vec4f& ray,
        QuaxolSpec* outPresentBlock, Vec4f* outPos);

    bool RayCastGround(const Vec4f& position, const Vec4f& direction, float* outDistance);
    void ClampToGround(Vec4f* position, Vec4f* velocity);

    bool RayCastChunk(const QuaxolChunk& chunk,
        const Vec4f& position, const Vec4f& ray, float* outDistance);
    bool LocalRayCastChunk(const QuaxolChunk& chunk,
        const Vec4f& start, const Vec4f& ray, Vec4f* outPos);

    void LineDraw4D(const Vec4f& start, const Vec4f& ray,
        DelegateN<void, int, int, int, int, const Vec4f&, const Vec4f&> callback);

    static void TestPhysics();
  protected:
    static TVecQuaxol s_testQuaxols;
    static void TestPhysicsCallback(int x, int y, int z, int w, const Vec4f& position, const Vec4f& ray);
  };
}; //namespace fd