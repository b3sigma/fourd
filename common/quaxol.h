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
  
  struct RenderBlock {
    enum DirIndex { 
      XPlusInd  = 0x0,
      XMinusInd = 0x1,
      YPlusInd  = 0x2,
      YMinusInd = 0x3,
      ZPlusInd  = 0x4,
      ZMinusInd = 0x5,
      WPlusInd  = 0x6,
      WMinusInd = 0x7,
      NumDirs,
    };

    enum Dir {
      XPlus     = (1 << XPlusInd),
      XMinus    = (1 << XMinusInd),
      YPlus     = (1 << YPlusInd),
      YMinus    = (1 << YMinusInd),
      ZPlus     = (1 << ZPlusInd),
      ZMinus    = (1 << ZMinusInd),
      WPlus     = (1 << WPlusInd),
      WMinus    = (1 << WMinusInd),
      NumDirCombinations = (1 << NumDirs),
    };
    unsigned char connectFlags;
  };

  // stb actually abandons indices
  // using 8 byte verts, 2 byte indices
  // tesseract: 16 verts * 8 bytes = 128 vert bytes
  // tri indices: 8 cubes * 6 faces * 2 tris * 3 indices * 2 bytes = 576 index bytes
  // just quads: 8 cubes * 6 faces * 1 quad * 8 bytes = 384 bytes
  // so tri indices + verts = 704 bytes
  // but tri quad verts = 384 bytes
  struct QuaxolVert {
    int _position; // 8 bits per x,y,z,w
    int _uv_color_ao; // 8 bits u,v, 8 bit color, 8 bit ao
  };
  typedef ::std::vector<QuaxolVert> QVertList;

  typedef ::std::vector<Vec4f> VecList;
  typedef ::std::vector<int> IndexList;
  
  struct CanonicalCube {
    VecList m_verts;
    IndexList m_indices;
    RenderBlock::Dir m_dir; // used by dir
    RenderBlock::DirIndex m_dirIndex; // used by dir
    unsigned char m_connectFlags; // used by flag

    typedef ::std::vector<unsigned char> VertDirs;
    static void populateVerts(float size, VecList& verts, VertDirs& vertDirs);
    static void addFlaggedTesseract(IndexList& indices, VertDirs& vertDirs, unsigned char flags);
    static void addFlaggedCube(IndexList& indices, VertDirs& vertDirs, unsigned char flags, int a, int b, int c, int d, int e, int f, int g, int h);
    static void addFlaggedQuad(IndexList& indices, VertDirs& vertDirs, unsigned char flags, int a, int b, int c, int d);
  };

  class QuaxolChunk {
  public:
    static const int c_mxSz = 16; // this means 16^4 blocks, or 32k
    // indexing [x][y][z][w] for the moment
    
    int m_cubeCount; // might not end up exact? suggestion
    RenderBlock m_connects[c_mxSz][c_mxSz][c_mxSz][c_mxSz];
    Block m_blocks[c_mxSz][c_mxSz][c_mxSz][c_mxSz];

    static CanonicalCube s_canonicalCubesByFlag[RenderBlock::NumDirCombinations];
    static CanonicalCube s_canonicalCubesByDir[RenderBlock::NumDirs];

    VecList m_verts;
    IndexList m_indices;

    Vec4f m_position;
    Vec4f m_blockSize; // this should probably live higher up?
    QuaxolSpec m_blockDims;

  public:
    QuaxolChunk(Vec4f position, Vec4f blockSize);
    ~QuaxolChunk();

    bool LoadFromList(const TVecQuaxol* pPresent, const QuaxolSpec* offset);
    
    // unchecked, local
    inline bool IsPresent(int x, int y, int z, int w) const { 
      assert(x >= 0 && x < c_mxSz && y >= 0 && y < c_mxSz && z >= 0 && z < c_mxSz && w >= 0 && w < c_mxSz);
      return m_blocks[x][y][z][w].present;
    }

    inline Block& GetBlock(int x, int y, int z, int w) { // unchecked, local
      return m_blocks[x][y][z][w];
    }
    
    inline bool IsValid(int x, int y, int z, int w) {
      return (x >= 0 && x < c_mxSz && y >= 0 && y < c_mxSz 
        && z >= 0 && z < c_mxSz && w >= 0 && w < c_mxSz);
    }

    // for now, always populate chunk edges
    inline int SetConnect(RenderBlock& rBlock, RenderBlock::Dir dir,
        int x, int y, int z, int w) {
      if(!IsValid(x, y, z, w))
        return 0;
      //if(!IsValid(x, y, z, w) || !IsPresent(x, y, z, w)) {
      if(!IsPresent(x, y, z, w)) {
        rBlock.connectFlags |= (unsigned char)(dir);
        return 1;
      } else {
        return 0;
      }
    }
 
    void UpdateConnects();
    void UpdateTrisFromConnects();

    void AddRenderCubeByDir(const Vec4f& vertOffset, RenderBlock::DirIndex dirIndex);
    void AddRenderCubeByFlag(const Vec4f& vertOffset, unsigned char connectFlags);
    static void BuildCanonicalCubesByDir(float blockSize);
    static void BuildCanonicalCubesByFlag(float blockSize);
  };

  class QuaxolScene {
  public:

  public:
    QuaxolScene() {}

  };



}; // namespace fd