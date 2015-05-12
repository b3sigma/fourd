#include <math.h>

#include "physics.h"
#include "physics_help.h"
#include "quaxol.h"

namespace fd {

void Physics::Step(float fDelta) {
}

// returns true if hit, overwrites distance with amount if so.
bool Physics::RayCast(
    const Vec4f& position, const Vec4f& ray, float* outDistance) {

  if(m_chunk) {
    if(RayCastChunk(*m_chunk, position, ray, outDistance)) {
      return true;
    }
  }
  return RayCastGround(position, ray, outDistance);
}

bool Physics::RayCastGround(
    const Vec4f& position, const Vec4f& ray, float* outDistance) {
  return PhysicsHelp::RayToPlane(position, ray,
      m_groundNormal, m_groundHeight,
      NULL /* outCollisionPoint */, outDistance);
}

void Physics::ClampToGround(Vec4f* position) {
  float dotGround = m_groundNormal.dot(*position);
  if(dotGround < 0.0f) {
    *position -= (m_groundNormal * dotGround);
  }
}

// Thinking 4d breseham line thing?
bool Physics::RayCastChunk(const QuaxolChunk& chunk,
    const Vec4f& position, const Vec4f& ray, float* outDistance) {
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
  // to avoid array checks on every block step, clip the ray to the possible
  // array bounds beforehand
  Vec4f unclippedLocalPos = localPos;
  Vec4f clippedPos;
  if(PhysicsHelp::RayToAlignedBox(chunkMin, chunkMax, localPos, localRay,
      NULL /*dist*/, &clippedPos)) {
    // if the start isn't within the box, clip that first
    if(!PhysicsHelp::WithinBox(chunkMin, chunkMax, localPos)) {
      localPos = clippedPos;
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
      localRay = clippedEnd - localPos;
      localRay *= 0.9999f; // shorten it a bit to avoid going over
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
  if(!LocalRayCastChunk(chunk, localPos, localRay, &localHitPos)) {
    return false;
  }
  //localHitPos *= chunk.m_blockSize;
  //localHitPos += chunk.m_position;

  if(outDistance) {
    Vec4f hitDelta = (localHitPos - unclippedLocalPos) * chunk.m_blockSize;
    *outDistance = abs(hitDelta.length());
  }

  return true;
}

inline int GetSign(float val) {
  if(signbit(val)) return -1;
  else return 1;
}

// presumes vec has N components
// Totally wasteful looking way to sort 4 things and get their indices
template <int N>
inline int GetAbsMaxComponent(const float* vec, int numExclusions, int exclude[N]) {
  float max = 0.0f;
  int maxIndex = 0;
  for(int c = 0; c < N; c++) {
    bool skip = false;
    for(int ex = 0; ex < numExclusions; ex++) {
      if(c == exclude[ex]) {
        skip = true;
        break;
      }
    }
    if (skip)
      continue;
    if(abs(vec[c]) >= max) {
      max = abs(vec[c]);
      maxIndex = c;
    }
  }
  return maxIndex;
}

bool Physics::LocalRayCastChunk(const QuaxolChunk& chunk,
    const Vec4f& position, const Vec4f& ray, Vec4f* outPos) {

  QuaxolSpec pos(position);
  QuaxolSpec end(position + ray);

  // extra axis indirection is to make generalization easier.
  int axis[4]; // 4!=24 ways of ordering this
  // fastest in code is probably a bunch of if statements
  // bit clearer but slower way
  axis[0] = GetAbsMaxComponent<4>(ray.raw(), 0, axis);
  axis[1] = GetAbsMaxComponent<4>(ray.raw(), 1, axis);
  axis[2] = GetAbsMaxComponent<4>(ray.raw(), 2, axis);
  axis[3] = GetAbsMaxComponent<4>(ray.raw(), 3, axis);

  //QuaxolSpec absRay(abs(ray.x), abs(ray.y), abs(ray.z), abs(ray.w));
  Vec4f absRay(abs(ray.x), abs(ray.y), abs(ray.z), abs(ray.w));
  float rayDist = ray.length();
  int countdown[3];
  // the primary direction absolute difference sets up the count
  if(absRay[axis[1]] != 0.0f) {
    // ceil means over-include boxes, which guarantees the line is included
    countdown[0] = (int)ceil((float)absRay[axis[0]] / (float)absRay[axis[1]]);
  } else {
    countdown[0] = (int)ceil(abs(rayDist));
  }
  if(absRay[axis[2]] != 0.0f) {
    countdown[1] = (int)ceil((float)absRay[axis[0]] / (float)absRay[axis[2]]);
  } else {
    countdown[1] = 0;
  }
  if(absRay[axis[3]] != 0.0f) {
    countdown[2] = (int)ceil((float)absRay[axis[0]] / (float)absRay[axis[3]]);
  } else {
    countdown[2] = 0;
  }

  int count[3];
  memcpy(&count, &countdown, sizeof(count));

  int step[4];
  step[axis[0]] = GetSign(ray[axis[0]]);
  step[axis[1]] = GetSign(ray[axis[1]]);
  step[axis[2]] = GetSign(ray[axis[2]]);
  step[axis[3]] = GetSign(ray[axis[3]]);

  // do the start
  if(chunk.IsPresent(pos[0], pos[1], pos[2], pos[3])
      && PhysicsHelp::RayToQuaxol(pos, position, ray, NULL /*outDist*/, outPos)) {
    return true;
  }

  // loop until we're at the end
  int totalSteps = abs(end[axis[0]] - pos[axis[0]]); 
  //while(pos != end) {
  for(int mainSteps = 0; mainSteps <= totalSteps; mainSteps++) {
    for(int counter = 0; counter < 3; counter++) {
      count[counter]--;
      if(count[counter] <= 0 && countdown[counter] > 0) {
        pos[axis[counter + 1]] += step[axis[counter + 1]];
        count[counter] = countdown[counter];
        if(chunk.IsPresent(pos[0], pos[1], pos[2], pos[3])
            && PhysicsHelp::RayToQuaxol(pos, position, ray, NULL /*outDist*/, outPos)) {
          return true;
        }
        if(pos == end) return false;
      }
    }

    pos[axis[0]] += step[axis[0]];
    if(chunk.IsPresent(pos[0], pos[1], pos[2], pos[3])
        && PhysicsHelp::RayToQuaxol(pos, position, ray, NULL /*outDist*/, outPos)) {
      return true;
    }
    if(pos == end) return false;
  }
 
  return false;
}

// normalized direction is converted into n ints for n-dim
// The algorithm is to progress along a specific direction and decrement
// each counter along the largest direction
// In 2d this means 4 different cardinal directions, but two different
// primary axes, each that can step in negative. But there are two different
// code paths
// In 3d this means 3*2*1 different code paths, or n! different code paths.
// This is made tractable with generalized indices.
// Harder version, draw a line in 4d
void Physics::LineDraw4D(const Vec4f& position, const Vec4f& ray, float* outDist,
    DelegateN<bool, const QuaxolChunk*, int, int, int, int> callback) {
  QuaxolSpec pos(position);
  QuaxolSpec end(position + ray);

  // extra axis indirection is to make generalization easier.
  int axis[4]; // 4!=24 ways of ordering this
  // fastest in code is probably a bunch of if statements
  // bit clearer but slower way
  axis[0] = GetAbsMaxComponent<4>(ray.raw(), 0, axis);
  axis[1] = GetAbsMaxComponent<4>(ray.raw(), 1, axis);
  axis[2] = GetAbsMaxComponent<4>(ray.raw(), 2, axis);
  axis[3] = GetAbsMaxComponent<4>(ray.raw(), 3, axis);

  float rayDist = ray.length();
  int countdown[3];
  // the primary direction absolute difference sets up the count
  if(ray[axis[1]] != 0.0f) {
    // ceil means over-include boxes, which guarantees the line is included
    countdown[0] = static_cast<int>(ceil(abs(ray[axis[0]] / ray[axis[1]])));
  } else {
    countdown[0] = static_cast<int>(ceil(abs(rayDist)));
  }
  if(ray[axis[2]] != 0.0f) {
    countdown[1] = static_cast<int>(ceil(abs(ray[axis[0]] / ray[axis[2]])));
  } else {
    countdown[1] = 0;
  }
  if(ray[axis[3]] != 0.0f) {
    countdown[2] = static_cast<int>(ceil(abs(ray[axis[0]] / ray[axis[3]])));
  } else {
    countdown[2] = 0;
  }

  int count[3];
  memcpy(&count, &countdown, sizeof(count));

  int step[4];
  step[axis[0]] = GetSign(ray[axis[0]]);
  step[axis[1]] = GetSign(ray[axis[1]]);
  step[axis[2]] = GetSign(ray[axis[2]]);
  step[axis[3]] = GetSign(ray[axis[3]]);

  // do the start
  callback(NULL, pos[0], pos[1], pos[2], pos[3]);
  // loop until we're at the end
  int totalSteps = abs(end[axis[0]] - pos[axis[0]]); 
  //while(pos != end) {
  for(int mainSteps = 0; mainSteps <= totalSteps; mainSteps++) {
    count[0]--;
    if(count[0] < 0 && countdown[0] > 0) {
      pos[axis[1]] += step[axis[1]];
      count[0] = countdown[0];
      callback(NULL, pos[0], pos[1], pos[2], pos[3]);
      if(pos == end) break;
    } 
    count[1]--;
    if(count[1] < 0 && countdown[1] > 0) {
      pos[axis[2]] += step[axis[2]];
      count[1] = countdown[1];
      callback(NULL, pos[0], pos[1], pos[2], pos[3]);
      if(pos == end) break;
    }
    count[2]--;
    if(count[2] < 0 && countdown[2] > 0) {
      pos[axis[3]] += step[axis[3]];
      count[2] = countdown[2];
      callback(NULL, pos[0], pos[1], pos[2], pos[3]);
      if(pos == end) break;
    }

    pos[axis[0]] += step[axis[0]];
    callback(NULL, pos[0], pos[1], pos[2], pos[3]);
    if(pos == end) break;
  }
}

// Easy version, draw a line in 2d
// dist is out, for when this becomes raycasting
void Physics::LineDraw2D(const Vec4f& position, const Vec4f& ray, float* outDist,
    DelegateN<bool, const QuaxolChunk*, int, int, int, int> callback) {
  int pos[2];
  pos[0] = static_cast<int>(floor(position.x));
  pos[1] = static_cast<int>(floor(position.y));

  int end[2];
  end[0] = static_cast<int>(floor(position.x + ray.x));
  end[1] = static_cast<int>(floor(position.y + ray.y));

  // extra axis indirection is to make generalization easier.
  int axis[2];
  if(abs(ray.x) > abs(ray.y)) {
    axis[0] = 0;
    axis[1] = 1;
  } else {
    axis[0] = 1;
    axis[1] = 0;
  }

  float rayDist = ray.length();
  int countdown[2];
  if(ray[axis[1]] == 0) {
    countdown[0] = static_cast<int>(abs(ceil(rayDist)));
    countdown[1] = 0;
  } else {
    // Ceil here seems right, as you want to over-include quaxols instead of under.
    countdown[0] = static_cast<int>(abs(ceil(ray[axis[0]] / ray[axis[1]])));
    countdown[1] = abs(pos[axis[1]] - end[axis[1]]);
  }

  int count[2];
  memcpy(&count, &countdown, sizeof(count));

  int step[2];
  step[axis[0]] = GetSign(ray[axis[0]]);
  step[axis[1]] = GetSign(ray[axis[1]]);
              //.
              //...
  while(pos[axis[0]] != end[axis[0]] || pos[axis[1]] != end[axis[1]]) {
    callback(NULL, pos[0], pos[1], 0, 0);
    count[0]--;
    if(count[0] < 0) {
      pos[axis[1]] += step[axis[1]];
      count[0] = countdown[0];
      count[1]--;
    } else {
      // don't ever step in two directions in the samer iteration
      // keep faces between quaxols instead of corner connections
      pos[axis[0]] += step[axis[0]];
    }
  }
  callback(NULL, end[0], end[1], 0, 0);
}

TVecQuaxol Physics::s_testQuaxols;
bool Physics::TestPhysicsCallback(const QuaxolChunk* chunk, int x, int y, int z, int w) {
  s_testQuaxols.emplace_back(x, y, z, w);
  return true;
}

void Physics::TestPhysics() {
  Physics physTest;

  ////////////////////////
  // Basic raycast
  Vec4f posTest(1.0f, 1.0f, 1.0f, 1.0f);
  Vec4f rayDown(0.0f, 0.0f, -10.0f, 0.0f);
  float dist = 0.0f;
  assert(true == physTest.RayCastGround(posTest, rayDown, &dist));
  assert(dist == 1.0f); // may need to do some float threshold compares

  posTest.set(20.0f, 0.0f, 11.0f, 0.0f);
  assert(false == physTest.RayCastGround(posTest, rayDown, &dist));

  posTest.set(10.0f, 0.0f, 3.0f, 0.0f);
  rayDown.set(8.0f, 0.0f, -6.0f, 0.0f);
  assert(true == physTest.RayCastGround(posTest, rayDown, &dist));
  assert(dist == 5.0f);

  /////////////////////
  // callback sanity
  DelegateN<bool, const QuaxolChunk*, int, int, int, int> stepCallback;
  stepCallback.Bind(Physics::TestPhysicsCallback);
  stepCallback(NULL, 1, 2, 0, 0);
  assert(s_testQuaxols.size() == 1);
  assert(s_testQuaxols[0].x == 1 && s_testQuaxols[0].y == 2);

  ////////////////////
  // LineDraw2D, which was supposed to be extended to 4d
  // as it's a nice algorithm
  s_testQuaxols.resize(0);
  Vec4f pos(1.5f, 1.5f, 0.0f, 0.0f);
  Vec4f ray(2.0f, 0.0f, 0.0f, 0.0f);
  physTest.LineDraw2D(pos, ray, &dist, stepCallback); 
  assert(s_testQuaxols.size() == 3);
  assert(s_testQuaxols[0].x == 1 && s_testQuaxols[0].y == 1);
  assert(s_testQuaxols[1].x == 2 && s_testQuaxols[1].y == 1);
  assert(s_testQuaxols[2].x == 3 && s_testQuaxols[2].y == 1);

  s_testQuaxols.resize(0);
  pos.set(1.5f, 1.5f, 0.0f, 0.0f);
  ray.set(-2.0f, 1.0f, 0.0f, 0.0f); // draw from (1.5,1.5) to (-0.5,2.5)
  physTest.LineDraw2D(pos, ray, &dist, stepCallback); 
  assert(s_testQuaxols.size() == 4); //(1,1), (0,1), (-1,1), (-1,2)
  assert(s_testQuaxols[0].x == 1 && s_testQuaxols[0].y == 1);
  //assert(s_testQuaxols[1].x == 2 && s_testQuaxols[1].y == 1);
  assert(s_testQuaxols[3].x == -1 && s_testQuaxols[3].y == 2);

  s_testQuaxols.resize(0);
  pos.set(1.5f, 1.5f, 0.0f, 0.0f);
  ray.set(-1.0f, -2.0f, 0.0f, 0.0f); // draw from (1.5,1.5) to (0.5,-0.5)
  physTest.LineDraw2D(pos, ray, &dist, stepCallback); 
  assert(s_testQuaxols.size() == 4); //(1,1), (1,0), (1,-1)
  assert(s_testQuaxols[0].x == 1 && s_testQuaxols[0].y == 1);
  //assert(s_testQuaxols[1].x == 2 && s_testQuaxols[1].y == 1);
  assert(s_testQuaxols[3].x == 0 && s_testQuaxols[3].y == -1);

  ///////////////////
  // 4D drawing
  pos.set(1.5f, 1.5f, 1.5f, 1.5f);
  ray.set(0.0f, 0.0f, 0.0f, 2.0f);
  s_testQuaxols.resize(0);
  physTest.LineDraw4D(pos, ray, &dist, stepCallback);
  assert(s_testQuaxols.size() == 3);
  assert(s_testQuaxols[0].w == 1 && s_testQuaxols[0].y == 1);
  assert(s_testQuaxols[1].w == 2 && s_testQuaxols[1].y == 1);
  assert(s_testQuaxols[2].w == 3 && s_testQuaxols[2].y == 1);

  pos.set(1.5f, 1.5f, 1.5f, 1.5f);
  ray.set(0.0f, 1.0f, 0.0f, -2.0f);
  s_testQuaxols.resize(0);
  physTest.LineDraw4D(pos, ray, &dist, stepCallback);
  assert(s_testQuaxols.size() == 4); //(1,1,1,1), (1,1,1,0), (1,1,1,-1), (1,2,1,-1)
  assert(s_testQuaxols[0] == QuaxolSpec(1,1,1,1));
  assert(s_testQuaxols[3] == QuaxolSpec(1,2,1,-1));

  pos.set(1.5f, 1.5f, 1.5f, 1.5f);
  ray.set(-2.5f, 1.0f, 1.7f, -2.0f);
  s_testQuaxols.resize(0);
  physTest.LineDraw4D(pos, ray, &dist, stepCallback);
  assert(s_testQuaxols.size() == 6);
  assert(s_testQuaxols[0] == QuaxolSpec(1,1,1,1));
  assert(s_testQuaxols[5] == QuaxolSpec(-2,1,2,0)); //is this even right?

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

  //quaxols.emplace_back(
}

} // namespace fd