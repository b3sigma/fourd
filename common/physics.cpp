#include <math.h>

#include "physics.h"
#include "quaxol.h"

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

// Thinking 4d breseham line thing?
bool Physics::RayCastChunk(const QuaxolChunk& chunk,
    const Vec4f& position, const Vec4f& direction, float* outDistance) {

  Vec4f localPos(position);
  localPos -= chunk.m_position;

  // normalized direction is converted into n ints for n-dim
  // The algorithm is to progress along a specific direction and decrement
  // each counter along the largest direction
  // In 2d this means 4 different cardinal directions, but two different
  // primary axes, each that can step in negative. But there are two different
  // code paths
  // In 3d this means 3*2*1 different code paths, or n! different code paths.
  // This can be done more generally with indices probably.

  return false;
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
  assert(true == physTest.RayCast(&posTest, &rayDown, &dist));
  assert(dist == 1.0f); // may need to do some float threshold compares

  posTest.set(20.0f, 0.0f, 11.0f, 0.0f);
  assert(false == physTest.RayCast(&posTest, &rayDown, &dist));

  posTest.set(10.0f, 0.0f, 3.0f, 0.0f);
  rayDown.set(8.0f, 0.0f, -6.0f, 0.0f);
  assert(true == physTest.RayCast(&posTest, &rayDown, &dist));
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

}

} // namespace fd