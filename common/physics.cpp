#include <math.h>

#include "physics.h"

namespace fd {

void Physics::Step(float fDelta) {
}

// returns true if hit, overwrites distance with amount if so.
bool Physics::RayCast(
    const Vec4f* position, const Vec4f* ray, float* outDistance) {
  return RayCastGround(position, ray, outDistance);
}

bool Physics::RayCastGround(
    const Vec4f* position, const Vec4f* ray, float* outDistance) {
  Vec4f endPos(*position);
  endPos += *ray;
  float posDotGround = m_groundNormal.dot(endPos);
  float startDotGround = m_groundNormal.dot(*position);
  if (posDotGround * startDotGround >= 0.0f) {
    return false; //both are on same side of ground, no collision
  } else {
    // opposite sides, collision
    if (outDistance) {
      *outDistance = ray->length() * startDotGround / (abs(posDotGround) + abs(startDotGround));
    }
    return true;
  }
}

void Physics::ClampToGround(Vec4f* position) {
  float dotGround = m_groundNormal.dot(*position);
  if(dotGround < 0.0f) {
    *position -= (m_groundNormal * dotGround);
  }
}

void Physics::TestPhysics() {
  Physics physTest;

  Vec4f posTest(1.0f, 1.0f, 1.0f, 1.0f);
  Vec4f rayDown(0.0f, 0.0f, -10.0f, 0.0f);
  float dist = 0.0f;
  assert(true == physTest.RayCast(&posTest, &rayDown, &dist));
  assert(dist == 1.0f); // may need to do some float threshold compares

  posTest.set(20.0f, 0.0f, 11.0f, 0.0f);
  assert(false == physTest.RayCast(&posTest, &rayDown, &dist));

  posTest.set(10.0f, 0.0f, 3.0f, 0.0f);
  rayDown.set(8.0f, 0.0f, -6.0f, 0.0f);
  assert(true == physTest.RayCast(&posTest, &rayDown, &dist));
  assert(dist == 5.0f);

}

} // namespace fd