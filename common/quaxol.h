#pragma once

#include <vector>
#include "fourmath.h"

namespace fd {
 
  // This specifies a block but isn't actually a quaxol,
  // as the space storage would suck.
  class QuaxolSpec {
  public:
    union {
      int p[4];
      struct {
        int x, y, z, w;
      };
    };
    QuaxolSpec() : x(0), y(0), z(0), w(0) {}
    QuaxolSpec(int inX, int inY, int inZ, int inW) : x(inX), y(inY), z(inZ), w(inW) {} 
    QuaxolSpec(const QuaxolSpec& copy) : x(copy.x), y(copy.y), z(copy.z), w(copy.w) {}
    QuaxolSpec(float fX, float fY, float fZ, float fW) {
      x = static_cast<int>(floor(fX));
      y = static_cast<int>(floor(fY));
      z = static_cast<int>(floor(fZ));
      w = static_cast<int>(floor(fW));
    }
    QuaxolSpec(const Vec4f& vec) : QuaxolSpec(vec.x, vec.y, vec.z, vec.w) {}

    inline QuaxolSpec& operator -= (const QuaxolSpec& r) {
      x -= r.x; y -= r.y; z -= r.z; w -= r.w;
      return *this;
    }
    inline bool operator == (const QuaxolSpec& r) {
      return x == r.x && y == r.y && z == r.z && w == r.w;
    }
    inline bool operator != (const QuaxolSpec& r) { return !(*this == r); }
    inline int& operator[] (int index) { return p[index]; }

    Vec4f ToFloatCoords(const Vec4f& offset, const Vec4f& blockSize) {
      Vec4f coord((float)x, (float)y, (float)z, (float)w);
      coord *= blockSize;
      coord += offset;
      return coord;
    }
  };
  typedef std::vector<QuaxolSpec> TVecQuaxol;


  struct Block {
    // bit pack this shortly, but let's get it running first
    bool present;
  };
  

  class QuaxolChunk {
  public:
    static const int c_mxSz = 16; // this means 16^4 blocks, or 32k
    // indexing [x][y][w][z] for the moment
    
    Block m_blocks[c_mxSz][c_mxSz][c_mxSz][c_mxSz];
    Vec4f m_position;
    Vec4f m_blockSize; // this should probably live higher up?
    QuaxolSpec m_blockDims;

  public:
    QuaxolChunk(Vec4f position, Vec4f blockSize);
    ~QuaxolChunk();

    bool LoadFromList(const TVecQuaxol* pPresent, const QuaxolSpec* offset);
    
    // unchecked, local
    inline bool IsPresent(int x, int y, int z, int w) const { 
      return m_blocks[x][y][w][z].present;
    }

    inline Block& GetBlock(int x, int y, int z, int w) { // unchecked, local
      return m_blocks[x][y][w][z];
    }
  };

}; // namespace fd