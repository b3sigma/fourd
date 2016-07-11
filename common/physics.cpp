#include <float.h>
#include <math.h>

#include "physics.h"
#include "physics_help.h"
#include "quaxol.h"

namespace fd {

void Physics::Step(float fDelta) {
}

bool Physics::RayCast(const Vec4f& position, const Vec4f& ray,
    float* outDistance, Vec4f* normal) {
  if(m_chunk) {
    if(RayCastChunk(*m_chunk, position, ray, outDistance, normal)) {
      return true;
    }
  }
  if(RayCastGround(position, ray, outDistance)) {
    if(normal) {
      *normal = m_groundNormal;
    }
    return true;
  }
  return false;
}

// returns true if hit, overwrites distance with amount if so.
bool Physics::RayCast(
    const Vec4f& position, const Vec4f& ray, float* outDistance) {
  return RayCast(position, ray, outDistance, NULL /*normal*/);
}


bool Physics::RayCastGround(
    const Vec4f& position, const Vec4f& ray, float* outDistance) {
  return PhysicsHelp::RayToPlane(position, ray,
      m_groundNormal, m_groundHeight,
      NULL /* outCollisionPoint */, outDistance);
}

bool Physics::ClampToGround(Vec4f* position, Vec4f* velocity) {
  float dotGround = m_groundNormal.dot(*position);
  if(dotGround < 0.0f) {
    *position -= (m_groundNormal * dotGround);

    float velDot = m_groundNormal.dot(*velocity);
    if(velDot < 0.0f) {
      *velocity -= (m_groundNormal * velDot);
    }
    return true;
  }
  return false;
}

bool Physics::SphereCollide(const Vec4f& position, float radius,
    Vec4f* hitPos, Vec4f* hitNormal) {

  if(m_chunk && SphereToQuaxols(*m_chunk, position, radius, hitPos, hitNormal)) {
    return true;
  }

  if(PhysicsHelp::SphereToPlane(position, radius,
      m_groundNormal, m_groundHeight, hitPos)) {
    if(hitNormal) {
      *hitNormal = m_groundNormal;
    }
    return true;
  }
  return false;
}

bool Physics::SphereToQuaxols(const QuaxolChunk& chunk,
    const Vec4f& position, float radius,
    Vec4f* hitPos, Vec4f* hitNormal) {

  Vec4f localPos(position);
  localPos -= chunk.m_position;
  localPos /= chunk.m_blockSize;

  // to support non-uniform blocksize, need local ellipse or world ellipse
  assert(chunk.m_blockSize.x == chunk.m_blockSize.y
      && chunk.m_blockSize.x == chunk.m_blockSize.w
      && chunk.m_blockSize.x == chunk.m_blockSize.z);
  float localRadius = radius / chunk.m_blockSize.x;

  Vec4f localHitPos;
  if(!LocalSphereToQuaxolChunk(chunk, localPos, localRadius,
      &localHitPos, hitNormal)) {
    return false;
  }

  localHitPos *= chunk.m_blockSize;
  localHitPos += chunk.m_position;
  if(hitPos)
    *hitPos = localHitPos;

  return true;
}

bool Physics::LocalSphereToQuaxolChunk(const QuaxolChunk& chunk,
    const Vec4f& position, float radius,
    Vec4f* hitPos, Vec4f* hitNormal) {

  Vec4f radiusVec(radius, radius, radius, radius);
  Vec4f min = position - radiusVec;
  Vec4f max = position + radiusVec;

  QuaxolSpec gridMin(min);
  QuaxolSpec gridMax(max);
  chunk.ClipToValid(gridMin);
  chunk.ClipToValid(gridMax);

  bool foundHit = false;
  float smallestHitSq = FLT_MAX;
  Vec4f bestHit;
  Vec4f bestNormal;

  Vec4f minBox;
  Vec4f maxBox;
  Vec4f currentHit;
  Vec4f currentNormal;
  for(int x = gridMin.x; x <= gridMax.x; ++x) {
    minBox.x = (float)x;
    maxBox.x = (float)(x + 1);
    for(int y = gridMin.y; y <= gridMax.y; ++y) {
      minBox.y = (float)y;
      maxBox.y = (float)(y + 1);
      for (int z = gridMin.z; z <= gridMax.z; ++z) {
        minBox.z = (float)z;
        maxBox.z = (float)(z + 1);
        for (int w = gridMin.w; w <= gridMax.w; ++w) {
          minBox.w = (float)w;
          maxBox.w = (float)(w + 1);

          if(!chunk.IsPresent(x, y, z, w))
            continue;

          //if(PhysicsHelp::SphereToAlignedBox(
          if(PhysicsHelp::SphereToAlignedBoxMinkowski(
              minBox, maxBox, position, radius, &currentHit, &currentNormal)) {
            float distSq = (position - currentHit).lengthSq();
            if(distSq < smallestHitSq) {
              smallestHitSq = distSq;
              bestHit = currentHit;
              bestNormal = currentNormal;
              foundHit = true;
            }
          }

        } // w
      } // z
    } // y
  } // x

  if(foundHit) {
    if(hitPos)
      *hitPos = bestHit;
    if(hitNormal)
      *hitNormal = bestNormal;
  }

  return foundHit;
}

bool Physics::RayCastChunk(const QuaxolChunk& chunk,
    const Vec4f& position, const Vec4f& ray, float* outDistance) {
  return RayCastChunk(chunk, position, ray, outDistance, NULL /*normal*/);
}

bool Physics::RayCastChunk(const QuaxolChunk& chunk,
    const Vec4f& position, const Vec4f& ray, float* outDistance, Vec4f* outNormal) {
  assert(ray.length() > 0.0f);

  Vec4f localPos(position);
  localPos -= chunk.m_position;

  Vec4f localRay(ray);

  // scale down from boxsize to allow integer local coords
  // actually why are we doing non-integer stuff anyway??
  // oh right, didn't start with quaxols... probably should change that
  // or forever suffer.
  localPos /= chunk.m_blockSize;
  localRay /= chunk.m_blockSize;

  Vec4f chunkMin(0.0f, 0.0f, 0.0f, 0.0f);
  Vec4f chunkMax((float)chunk.m_blockDims.x, (float)chunk.m_blockDims.y,
      (float)chunk.m_blockDims.z, (float)chunk.m_blockDims.w);
  const float edgeThreshold = 0.001f; // ok these are getting silly...
  Vec4f floatThresholdMax(edgeThreshold, edgeThreshold, edgeThreshold, edgeThreshold);
  chunkMax -= floatThresholdMax;
  chunkMin += floatThresholdMax;
  // to avoid array checks on every block step, clip the ray to the possible
  // array bounds beforehand
  Vec4f unclippedLocalPos = localPos;
  Vec4f clippedPos;
  if(PhysicsHelp::RayToAlignedBox(chunkMin, chunkMax, localPos, localRay,
      NULL /*dist*/, &clippedPos)) {
    // if the start isn't within the box, clip that first
    if(!PhysicsHelp::WithinBox(chunkMin, chunkMax, localPos)) {
      Vec4f normalLocalRay = localRay.normalized();
      localPos = clippedPos + (normalLocalRay * edgeThreshold);
    }

    // if the end isn't within the box, clip that
    if(!PhysicsHelp::WithinBox(chunkMin, chunkMax, localPos + localRay)) {
      Vec4f clippedEnd;
      // Need to clip ray too so raycast backward. Almost certainly already
      // calculated and discarded the result.
      if(!PhysicsHelp::RayToAlignedBox(chunkMin, chunkMax, localPos + localRay, -localRay,
          NULL /*dist*/, &clippedEnd)) {
        return false; // ???
      }
      //Vec4f normalUnclippedRay = localRay.normalized();
      localRay = clippedEnd - localPos;
      localRay *= 1.0f - edgeThreshold; // shorten it a bit to avoid going over

      Vec4f normalLocalRay = localRay.normalized();
      //assert(normalUnclippedRay.approxEqual(normalLocalRay, 0.000001f));
    }
  } else {
    // The ray didn't hit the bounding box, so make sure the start is within
    // or we are done because the whole chunk was missed.
    if(!PhysicsHelp::WithinBox(chunkMin, chunkMax, localPos)) {
      return false;
    }
  }

  // This does a stepping algorithm checking against quaxols in the ray path.
  Vec4f localHitPos;
  Vec4f localHitNormal;
  if(!LocalRayCastChunk(chunk, localPos, localRay,
      &localHitPos, &localHitNormal)) {
    return false;
  }
  //localHitPos *= chunk.m_blockSize;
  //localHitPos += chunk.m_position;

  if(outDistance) {
    Vec4f hitDelta = (localHitPos - unclippedLocalPos) * chunk.m_blockSize;
    *outDistance = abs(hitDelta.length());
  }
  if(outNormal) {
    *outNormal = localHitNormal;
  }

  return true;
}

bool Physics::RayCastToOpenQuaxol(const Vec4f& position, const Vec4f& ray,
    QuaxolSpec* outOpenBlock, Vec4f* outPos) {
  if(!m_chunk) return false;

  float outDist = FLT_MAX;
  if(!RayCastChunk(*m_chunk, position, ray, &outDist)) {
    if(!RayCastGround(position, ray, &outDist)) {
      return false;
    }
  }

  Vec4f hitPos = position + ray.normalized() * (outDist * 0.9999f);

  Vec4f cellPos(hitPos);
  cellPos = cellPos / m_chunk->m_blockSize;
  QuaxolSpec gridPos(cellPos);

  if(outOpenBlock) {
    *outOpenBlock = gridPos;
  }

  if(outPos) {
    *outPos = gridPos.ToFloatCoords(Vec4f(), m_chunk->m_blockSize);
  }
  return true;
}

bool Physics::RayCastToPresentQuaxol(const Vec4f& position, const Vec4f& ray,
    QuaxolSpec* outPresentBlock, Vec4f* outPos) {
  if(m_chunk) {
    float outDist = FLT_MAX;
    if(RayCastChunk(*m_chunk, position, ray, &outDist)) {
      float offset = 1.00001f;
      Vec4f hitPos = position + ray.normalized() * (outDist * offset);

      Vec4f cellPos(hitPos);
      cellPos = cellPos / m_chunk->m_blockSize;
      QuaxolSpec gridPos(cellPos);

      if(outPresentBlock) {
        *outPresentBlock = gridPos;
      }

      if(outPos) {
        *outPos = gridPos.ToFloatCoords(Vec4f(), m_chunk->m_blockSize);
      }
      return true;
    }
  }
  return false;

}


inline int GetSign(float val) {
  if(std::signbit(val)) return -1;
  else return 1;
}

inline void ClampToInteger(const Vec4f& position, const int (&signs)[4],
    Vec4f* outClampDelta, QuaxolSpec* outClampPos) {
  Vec4f clampPos;
  for (int c = 0; c < 4; c++) {
    clampPos[c] = (signs[c] < 0) ? ceil(position[c]) : floor(position[c]);
  }
  if(outClampDelta) {
    *outClampDelta = clampPos - position;
  }
  if(outClampPos) {
    *outClampPos = QuaxolSpec(clampPos);
  }
}

#define SAFE_BOUNDARY

bool Physics::LocalRayCastChunk(const QuaxolChunk& chunk,
    const Vec4f& start, const Vec4f& ray,
    Vec4f* outPos, Vec4f* outNormal) {

  int sign[4];
  for(int c = 0; c < 4; c++) {
    sign[c] = GetSign(ray[c]);
  }

  Vec4f normal = ray.normalized();

  Vec4f clampDir;
  ClampToInteger(start, sign, &clampDir, NULL /*gridPos*/);
  Vec4f step;
  Vec4f stepCounter;
  for(int c = 0; c < 4; c++) {
    if (normal[c] != 0.0f) {
      // we will add this to the counter whenever we take a step
      step[c] = abs(1.0f / normal[c]);
       // start out with the right number of "steps" based on start position
      if(clampDir[c] != 0.0f) {
        stepCounter[c] = (1.0f - abs(clampDir[c])) * step[c];
      } else {
        if(sign[c] < 0.0f) {
          stepCounter[c] = 0.0f;
        } else {
          stepCounter[c] = step[c];
        }
      }
   } else {
      step[c] = 0.0f;
      stepCounter[c] = FLT_MAX;
    }
  }

  QuaxolSpec gridPos(start);
  QuaxolSpec gridEnd(start + ray);

  // do the start
  if(chunk.IsPresent(gridPos[0], gridPos[1], gridPos[2], gridPos[3])) {
    // Actually back up the start by a little bit so if we had beed clipped
    // to avoid a chunk memory out of bounds error, we still hit correctly.
    const float shiftAmount = 0.01f;
    Vec4f shiftedStart = start - (normal * shiftAmount);
    Vec4f shiftedRay = ray + (normal * (2.0f * shiftAmount));
    if(!PhysicsHelp::RayToQuaxol(
        gridPos, shiftedStart, shiftedRay, NULL /*outDist*/, outPos, outNormal)) {
      // probably started within a block and a tiny ray
      *outPos = start;
    }
    return true;
  }

  while(gridPos != gridEnd) {
    int nextAxis;
    float smallestStep = FLT_MAX;
    for(int c = 0; c < 4; c++) {
      if(stepCounter[c] < smallestStep) {
        nextAxis = c;
        smallestStep = stepCounter[c];
      }
    }

    gridPos[nextAxis] += sign[nextAxis];
    stepCounter[nextAxis] += step[nextAxis];

#ifdef SAFE_BOUNDARY
    if(!chunk.IsValid(gridPos)) {
      return false;
    }
#endif // SAFE_BOUNDARY

    if(chunk.IsPresent(gridPos[0], gridPos[1], gridPos[2], gridPos[3])
        && PhysicsHelp::RayToQuaxol(gridPos, start, ray, NULL /*outDist*/, outPos, outNormal)) {
      return true;
    }
  }

  return false;
}

void Physics::LineDraw4D(const Vec4f& start, const Vec4f& ray,
    DelegateN<void, int, int, int, int, const Vec4f&, const Vec4f&> callback) {

  int sign[4];
  for(int c = 0; c < 4; c++) {
    sign[c] = GetSign(ray[c]);
  }

  Vec4f normal = ray.normalized();

  Vec4f clampDir;
  ClampToInteger(start, sign, &clampDir, NULL /*gridPos*/);
  Vec4f step;
  Vec4f stepCounter;
  for(int c = 0; c < 4; c++) {
    if (normal[c] != 0.0f) {
      // we will add this to the counter whenever we take a step
      step[c] = abs(1.0f / normal[c]);
       // start out with the right number of "steps" based on start position
      stepCounter[c] = (1.0f - abs(clampDir[c])) * step[c];
    } else {
      step[c] = 0.0f;
      stepCounter[c] = FLT_MAX;
    }
  }

  QuaxolSpec gridPos(start);
  QuaxolSpec gridEnd(start + ray);
  callback(gridPos[0], gridPos[1], gridPos[2], gridPos[3], start, ray);

  while(gridPos != gridEnd) {
    int nextAxis;
    float smallestStep = FLT_MAX;
    for(int c = 0; c < 4; c++) {
      if(stepCounter[c] < smallestStep) {
        nextAxis = c;
        smallestStep = stepCounter[c];
      }
    }

    gridPos[nextAxis] += sign[nextAxis];
    stepCounter[nextAxis] += step[nextAxis];
    callback(gridPos[0], gridPos[1], gridPos[2], gridPos[3], start, ray);
  }
}

TVecQuaxol Physics::s_testQuaxols;
void Physics::TestPhysicsCallback(
    int x, int y, int z, int w, const Vec4f& position, const Vec4f& ray) {
  s_testQuaxols.emplace_back(x, y, z, w);
  assert(true == PhysicsHelp::RayToQuaxol(
      QuaxolSpec(x, y, z, w), position, ray, NULL /*dist*/, NULL /*outPoint*/, NULL /*outNormal*/));
  const int infiniteCheck = 100;
  assert(x >= -infiniteCheck && x < infiniteCheck);
  assert(y >= -infiniteCheck && y < infiniteCheck);
  assert(z >= -infiniteCheck && z < infiniteCheck);
  assert(w >= -infiniteCheck && w < infiniteCheck);
}

void Physics::RunTests() {
  Physics physTest;

  ////////////////////////
  // Basic raycast
  Vec4f posTest(1.0f, 1.0f, 1.0f, 1.0f);
  //Vec4f rayDown(0.0f, 0.0f, -10.0f, 0.0f);
  Vec4f rayDown(0.0f, -10.0f, 0.0f, 0.0f);
  float dist = 0.0f;
  assert(true == physTest.RayCastGround(posTest, rayDown, &dist));
  assert((float)abs((float)(dist - 1.0f)) < 0.00001f); // may need to do some float threshold compares

  //posTest.set(20.0f, 0.0f, 11.0f, 0.0f);
  posTest.set(20.0f, 11.0f, 0.0f, 0.0f);
  assert(false == physTest.RayCastGround(posTest, rayDown, &dist));

  //posTest.set(10.0f, 0.0f, 3.0f, 0.0f);
  //rayDown.set(8.0f, 0.0f, -6.0f, 0.0f);
  posTest.set(10.0f, 3.0f, 0.0f, 0.0f);
  rayDown.set(8.0f, -6.0f, 0.0f, 0.0f);
  assert(true == physTest.RayCastGround(posTest, rayDown, &dist));
  assert(dist == 5.0f);

  /////////////////////
  // callback sanity
  DelegateN<void, int, int, int, int, const Vec4f&, const Vec4f&> stepCallback;
  stepCallback.Bind(Physics::TestPhysicsCallback);
  Vec4f pos(1.5f, 2.5f, 0.0f, 0.0f);
  Vec4f ray(2.0f, 0.0f, 0.0f, 0.0f);
  stepCallback(1, 2, 0, 0, pos, ray);
  assert(s_testQuaxols.size() == 1);
  assert(s_testQuaxols[0].x == 1 && s_testQuaxols[0].y == 2);

  ////////////////////
  // LineDraw2D, which was along the way to 4d
  // as it's a nice algorithm
  s_testQuaxols.resize(0);
  pos.set(1.5f, 1.5f, 0.0f, 0.0f);
  ray.set(2.0f, 0.0f, 0.0f, 0.0f);
  physTest.LineDraw4D(pos, ray, stepCallback);
  assert(s_testQuaxols.size() == 3);
  assert(s_testQuaxols[0].x == 1 && s_testQuaxols[0].y == 1);
  assert(s_testQuaxols[1].x == 2 && s_testQuaxols[1].y == 1);
  assert(s_testQuaxols[2].x == 3 && s_testQuaxols[2].y == 1);

  s_testQuaxols.resize(0);
  pos.set(1.5f, 1.5f, 0.0f, 0.0f);
  ray.set(-2.0f, 1.0f, 0.0f, 0.0f); // draw from (1.5,1.5) to (-0.5,2.5)
  physTest.LineDraw4D(pos, ray, stepCallback);
  assert(s_testQuaxols.size() == 4); //(1,1), (0,1), (-1,1), (-1,2)
  assert(s_testQuaxols[0].x == 1 && s_testQuaxols[0].y == 1);
  //assert(s_testQuaxols[1].x == 2 && s_testQuaxols[1].y == 1);
  assert(s_testQuaxols[3].x == -1 && s_testQuaxols[3].y == 2);

  s_testQuaxols.resize(0);
  pos.set(1.5f, 1.5f, 0.0f, 0.0f);
  ray.set(-1.0f, -2.0f, 0.0f, 0.0f); // draw from (1.5,1.5) to (0.5,-0.5)
  physTest.LineDraw4D(pos, ray, stepCallback);
  assert(s_testQuaxols.size() == 4); //(1,1), (1,0), (1,-1)
  assert(s_testQuaxols[0].x == 1 && s_testQuaxols[0].y == 1);
  //assert(s_testQuaxols[1].x == 2 && s_testQuaxols[1].y == 1);
  assert(s_testQuaxols[3].x == 0 && s_testQuaxols[3].y == -1);

  ///////////////////
  // 4D drawing
  pos.set(1.5f, 1.5f, 1.5f, 1.5f);
  ray.set(0.0f, 0.0f, 0.0f, 2.0f);
  s_testQuaxols.resize(0);
  physTest.LineDraw4D(pos, ray, stepCallback);
  assert(s_testQuaxols.size() == 3);
  assert(s_testQuaxols[0].w == 1 && s_testQuaxols[0].y == 1);
  assert(s_testQuaxols[1].w == 2 && s_testQuaxols[1].y == 1);
  assert(s_testQuaxols[2].w == 3 && s_testQuaxols[2].y == 1);

  pos.set(1.5f, 1.5f, 1.5f, 1.5f);
  ray.set(0.0f, 1.0f, 0.0f, -2.0f);
  s_testQuaxols.resize(0);
  physTest.LineDraw4D(pos, ray, stepCallback);
  assert(s_testQuaxols.size() == 4); //(1,1,1,1), (1,1,1,0), (1,1,1,-1), (1,2,1,-1)
  assert(s_testQuaxols[0] == QuaxolSpec(1,1,1,1));
  assert(s_testQuaxols[3] == QuaxolSpec(1,2,1,-1));

  pos.set(1.5f, 1.5f, 1.5f, 1.5f);
  ray.set(-2.5f, 1.0f, 1.7f, -2.0f);
  s_testQuaxols.resize(0);
  physTest.LineDraw4D(pos, ray, stepCallback);
  assert(s_testQuaxols.size() == 8);
  assert(s_testQuaxols[0] == QuaxolSpec(1,1,1,1));
  assert(s_testQuaxols[7] == QuaxolSpec(-1,2,3,-1));

  pos.set(0.0500000007f, 0.0500000007f, 1.54999995f, 0.449999988f);
  ray.set(0.319010586f, 15.9484043f, 7.15184228e-007f, 0.000000000f);
  s_testQuaxols.resize(0);
  physTest.LineDraw4D(pos, ray, stepCallback);
  // just need to make sure it terminates

  /////////// chunk raycasting //////////

  Vec4f chunkPos(0.0f, 0.0f, 0.0f, 0.0f);
  Vec4f chunkBlockSize(10.0f, 10.0f, 10.0f, 10.0f); //dumb to even support
  QuaxolChunk testChunk(chunkPos, chunkBlockSize);
  TVecQuaxol quaxols;
  quaxols.emplace_back(1, 1, 1, 1);
  assert(true == testChunk.LoadFromList(&quaxols, NULL /*offset*/));

  pos.set(25.0f, 25.0f, 25.0f, 25.0f);
  ray.set(-10.0f, -10.0f, -11.0f, -12.0f);
  assert(true == physTest.RayCastChunk(testChunk, pos, ray, NULL));

  quaxols.emplace_back(1, 2, 1, 1);
  quaxols.emplace_back(2, 1, 1, 1);
  quaxols.emplace_back(2, 2, 1, 1);
  quaxols.emplace_back(1, 1, 2, 1);
  quaxols.emplace_back(1, 1, 3, 1);
  quaxols.emplace_back(1, 1, 4, 1);
  quaxols.emplace_back(2, 2, 0, 2);
  assert(true == testChunk.LoadFromList(&quaxols, NULL /*offset*/));
  pos.set(25.0f, 25.0f, 25.0f, 25.0f);
  ray.set(-10.0f, -10.0f, 11.0f, -12.0f);
  float hitDist = -1.0f;
  assert(true == physTest.RayCastChunk(testChunk, pos, ray, &hitDist));
  assert(hitDist > 0.0f);

  ray.set(-1000.0f, -1000.0f, 1100.0f, -1200.0f);
  assert(true == physTest.RayCastChunk(testChunk, pos, ray, &hitDist));
  assert(hitDist > 0.0f);

  ray.set(-1000.0f, 0.01f, 0.01f, 0.01f);
  assert(false == physTest.RayCastChunk(testChunk, pos, ray, &hitDist));

  ray.set(0.01f, 0.01f, 0.01f, 0.01f);
  assert(false == physTest.RayCastChunk(testChunk, pos, ray, &hitDist));

  ray.set(0.01f, 0.01f, -10000.01f, 0.01f);
  assert(true == physTest.RayCastChunk(testChunk, pos, ray, &hitDist));
  assert(hitDist > 0.0f);

  pos.set(0.500000007f, 0.500000007f, 15.4999995f, 4.49999988f);
  ray.set(3.19010586f, 159.484043f, 7.15184228e-006f, 0.000000000f);
  assert(false == physTest.RayCastChunk(testChunk, pos, ray, &hitDist));

  // where do all these come frome? these are steps along the apparently long
  // road of writing a 4d raycaster that doesn't fuck up.
  quaxols.emplace_back(2, 0, 3, 0);
  assert(true == testChunk.LoadFromList(&quaxols, NULL /*offset*/));
  pos.set(25.7389202f, 9.39026356f, 36.6651955f, 4.50000000f);
  ray.set(-432.569427f, -739.241638f, -516.141785f, -0.000000000f);
  assert(true == physTest.RayCastChunk(testChunk, pos, ray, &hitDist));

  pos.set(0.500032365f, 82.7353592f, 142.041000f, 167.218506f);
  ray.set(-0.000243353134f, 426.009521f, -328.929626f, -842.804993f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);
  // just that it didn't crash

  pos.set(-0.774945676f, 34.8522301f, 39.2764893f, 108.281807f);
  ray.set(780.614685f, -79.4120407f, -619.972534f, 0.000123921200f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);
  // just that it didn't crash

  pos.set(-0.0543091893f, -0.0551338196f, 16.1201344f, 4.50000000f);
  ray.set(554.309204f, 555.133789f, -620.133728f, -0.000000000f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);

  pos.set(0.500000000f, 0.500000000f, 15.5000000f, 4.50000000f);
  ray.set(-701.758179f, -701.228516f, -125.758827f, -0.000000000f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);

  pos.set(40.5596008f, 19.8676262f, 0.000000000f, 4.50000000f);
  ray.set(1.00000000f, 0.000000000f, -0.317290395f, 0.000000000f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);

  pos.set(0.500000000f, 0.500000000f, 15.5000000f, 4.50000000f);
  ray.set(69.9288635f, 997.351501f, -19.9986229f, -0.000000000f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);

  pos.set(38.5371017f, 15.0098648f, 20.0000000f, 9.22431374f);
  ray.set(10.0000000f, 0.000000000f, -0.000565912109f, 0.000000000f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);

  pos.set(0.500000000f, 0.500000000f, 20.0000000f, 4.50000000f);
  ray.set(10.0000000f, 0.000000000f, 5.34876919f, 0.000000000f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);

  pos.set(42.2373123f, 28.4113293f, 0.000000000f, 4.50000000f);
  ray.set(0.000000000f, 0.000000000f, -7.50906911e-005f, -10.0000000f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);

  pos.set(40.0000038f, 63.3713875f, 9.99999905f, 4.50000000f);
  ray.set(-4.76829791e-006f, 0.000000000f, 9.53674316e-007f, 10.0000000f);
  physTest.RayCastChunk(testChunk, pos, ray, &hitDist);

  /////////// sphere //////////////
  quaxols.clear();
  quaxols.emplace_back(1, 1, 1, 1);
  quaxols.emplace_back(1, 2, 1, 1);
  quaxols.emplace_back(2, 1, 1, 1);
  quaxols.emplace_back(2, 2, 1, 1);
  assert(true == testChunk.LoadFromList(&quaxols, NULL /*offset*/));

  pos.set(15.0f, 15.0f, 20.0f, 15.0f);
  float radius = 20.0f;
  Vec4f hitPos;
  Vec4f hitNormal;
  assert(true == physTest.SphereToQuaxols(testChunk, pos, radius, &hitPos, &hitNormal));

  pos.set(15.0f, 15.0f, 30.0f, 15.0f);
  assert(true == physTest.SphereToQuaxols(testChunk, pos, radius, &hitPos, &hitNormal));

  pos.set(15.0f, 15.0f, 40.0f, 15.0f);
  assert(false == physTest.SphereToQuaxols(testChunk, pos, radius, &hitPos, &hitNormal));
}

} // namespace fd
