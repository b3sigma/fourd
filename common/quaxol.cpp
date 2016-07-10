#include <memory>

#include "quaxol.h"

#include "chunkloader.h"
#include "fourmath.h"
#include "mesh.h"

namespace fd {

QuaxolChunk::QuaxolChunk(Vec4f position, Vec4f blockSize)
    : m_position(position)
    , m_blockSize(blockSize)
    , m_blockDims(c_mxSz, c_mxSz, c_mxSz, c_mxSz) {
}

QuaxolChunk::~QuaxolChunk() {}

bool QuaxolChunk::LoadFromList(const TVecQuaxol* pPresent, const QuaxolSpec* offset) {
  static unsigned char startFilled = 0; //0xff
  memset(&m_blocks, startFilled, sizeof(m_blocks));

  if(!pPresent)
    return false;

  // This allows blocks to be loaded from anywhere and then made in local coords
  // for this specific chunk.
  QuaxolSpec origin(0,0,0,0);
  if(offset != NULL) {
    origin = *offset;
  }

  for(auto block : *pPresent) {
    QuaxolSpec local(block);
    local -= origin; // translate into chunk local coords.
    if(local.x < 0 || local.x >= c_mxSz
        || local.y < 0 || local.y >= c_mxSz
        || local.z < 0 || local.z >= c_mxSz
        || local.w < 0 || local.w >= c_mxSz) {
      printf("Block out of bounds: o:(%d %d %d %d) block:(%d %d %d %d)\n",
        origin.x, origin.y, origin.z, origin.w, local.x, local.y, local.z, local.w);
      continue;
    }

    int autoType = (local.w * 4 + (local.x % 3)) % 3;

    Block& fetchBlock = GetBlock(local.x, local.y, local.z, local.w);
    fetchBlock.present = !startFilled;
    fetchBlock.type = autoType;
  }

  UpdateRendering();

  return true;
}

void QuaxolChunk::UpdateRendering() {
  UpdateConnects();
  UpdateTrisFromConnects();
}

void QuaxolChunk::SetAt(const QuaxolSpec& pos, bool present) {
  if(!IsValid(pos)) return;
  Block& block = GetBlock(pos);
  block.present = present;
}

void QuaxolChunk::SetAt(const QuaxolSpec& pos, bool present, int type) {
  if(!IsValid(pos)) return;
  Block& block = GetBlock(pos);
  block.present = present;
  block.type = (unsigned char)type;
}

void QuaxolChunk::UpdateConnects() {
  // do this the slow but simple way as a first pass
  // index x,y,w,z currently, so do that loop
  m_cubeCount = 0;
  for (int x = 0; x < c_mxSz; ++x) {
    for (int y = 0; y < c_mxSz; ++y) {
      for (int w = 0; w < c_mxSz; ++w) {
        for (int z = 0; z < c_mxSz; ++z) {
          RenderBlock& rBlock = m_connects[x][y][z][w];
          rBlock.connectFlags = 0;
          if (!IsPresent(x, y, z, w)) {
            continue;
          }
          //else {
          //  rBlock.connectFlags = 0xff;
          //  continue;
          //}

          m_cubeCount += SetConnect(rBlock, RenderBlock::XPlus,  x+1, y, z, w);
          m_cubeCount += SetConnect(rBlock, RenderBlock::XMinus, x-1, y, z, w);
          m_cubeCount += SetConnect(rBlock, RenderBlock::YPlus,  x, y+1, z, w);
          m_cubeCount += SetConnect(rBlock, RenderBlock::YMinus, x, y-1, z, w);
          m_cubeCount += SetConnect(rBlock, RenderBlock::ZPlus,  x, y, z+1, w);
          m_cubeCount += SetConnect(rBlock, RenderBlock::ZMinus, x, y, z-1, w);
          m_cubeCount += SetConnect(rBlock, RenderBlock::WPlus,  x, y, z, w+1);
          m_cubeCount += SetConnect(rBlock, RenderBlock::WMinus, x, y, z, w-1);
        }
      }
    }
  }
}

void CanonicalCube::addFlaggedQuad(IndexList& indices, VertDirs& vertDirs, unsigned char flags, int a, int b, int c, int d) {
  unsigned char commonFlag = vertDirs[a] & vertDirs[b] & vertDirs[c] & vertDirs[d];
  if (commonFlag & flags) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);

    indices.push_back(c);
    indices.push_back(b);
    indices.push_back(d);
  }
}

void CanonicalCube::addFlaggedCube(IndexList& indices, VertDirs& vertDirs, unsigned char flags, int a, int b, int c, int d, int e, int f, int g, int h) {
  addFlaggedQuad(indices, vertDirs, flags, a, b, c, d);
  addFlaggedQuad(indices, vertDirs, flags, a, b, e, f);
  addFlaggedQuad(indices, vertDirs, flags, a, c, e, g);
  addFlaggedQuad(indices, vertDirs, flags, b, d, f, h);
  addFlaggedQuad(indices, vertDirs, flags, c, d, g, h);
  addFlaggedQuad(indices, vertDirs, flags, e, f, g, h);
}

//          |   \\       \?
//          |     \\      \?
// --> x    v y     v z    \?
//                          v w
// 00  01
// 02  03
//   04  05
//   06  07
//  08  09
//  10  11
//    12  13
//    14  15
// the premise is that not only are whole cubes clipped
// but also faces on adjacent cubes
// probably going to realize this is hopelessly wrong about 10 seconds
// after I see the results for the first time
void CanonicalCube::addFlaggedTesseract(IndexList& indices, VertDirs& vertDirs, unsigned char flags) {
  static bool bOverride = false;
  if(flags & RenderBlock::XPlus || bOverride) {
    addFlaggedCube(indices, vertDirs, flags, 1, 3, 5, 7, 9, 11, 13, 15); // x plus
  }
  if(flags & RenderBlock::XMinus || bOverride) {
    addFlaggedCube(indices, vertDirs, flags, 0, 2, 4, 6, 8, 10, 12, 14); // x minus
  }
  if (flags & RenderBlock::YPlus || bOverride) {
    addFlaggedCube(indices, vertDirs, flags, 2, 3, 6, 7, 10, 11, 14, 15); // y plus
  }
  if (flags & RenderBlock::YMinus || bOverride) {
    addFlaggedCube(indices, vertDirs, flags, 0, 1, 4, 5, 8, 9, 12, 13); // y minus
  }
  if (flags & RenderBlock::ZPlus || bOverride) {
    addFlaggedCube(indices, vertDirs, flags, 4, 5, 6, 7, 12, 13, 14, 15); // z plus
  }
  if (flags & RenderBlock::ZMinus || bOverride) {
    addFlaggedCube(indices, vertDirs, flags, 0, 1, 2, 3, 8, 9, 10, 11); // z minus
  }
  if (flags & RenderBlock::WPlus || bOverride) {
    addFlaggedCube(indices, vertDirs, flags, 8, 9, 10, 11, 12, 13, 14, 15); // w plus
  }
  if (flags & RenderBlock::WMinus || bOverride) {
    addFlaggedCube(indices, vertDirs, flags, 0, 1, 2, 3, 4, 5, 6, 7); // w minus
  }
}

void CanonicalCube::populateVerts(float size, QVertList& packVerts, VecList& verts, VertDirs& vertDirs) {
  const int dim = 4;
  int numVerts = 1 << dim;
  packVerts.resize(0);
  packVerts.reserve(numVerts);
  verts.resize(0);
  verts.reserve(numVerts);
  vertDirs.resize(0);
  vertDirs.reserve(numVerts);
  for (int i = 0; i < numVerts; i++) {
    int possibleDim = dim - 1;
    const int& mask = i;

    QuaxolVert packVert;
    packVert._position = 0;
    packVert._uv_ao = 0;
    Vec4f vert;

    unsigned char dirs = 0;
    while (possibleDim >= 0) {
      if (mask & (1 << possibleDim)) {
        packVert._position |= (1 << (possibleDim * c_quaxolPositionBits));
        vert.set(possibleDim, size);
        dirs |= (1 << ((possibleDim * 2) + 0));
      } else {
        dirs |= (1 << ((possibleDim * 2) + 1));
      }
      possibleDim--;
    }

    verts.push_back(vert);
    vertDirs.push_back(dirs);
    packVerts.push_back(packVert);
  }
}

CanonicalCube QuaxolChunk::s_canonicalCubesByFlag[RenderBlock::NumDirCombinations];
void QuaxolChunk::BuildCanonicalCubesByFlag(float blockSize) {
  VecList tessVerts;
  QVertList packVerts;
  CanonicalCube::VertDirs vertDirs;
  assert(sizeof(QuaxolVert) == 4);
  CanonicalCube::populateVerts(blockSize, packVerts, tessVerts, vertDirs);

  IndexList indices;
  indices.reserve(8 * 6 * 3 * 2);

  typedef std::map<int, int> OldToNewHash;
  OldToNewHash oldToNew;

  for(int flag = 0; flag < RenderBlock::NumDirCombinations; ++flag) {
    indices.resize(0);
    CanonicalCube::addFlaggedTesseract(indices, vertDirs, flag);

    CanonicalCube& cube = s_canonicalCubesByFlag[flag];
    cube.m_connectFlags = flag;

    // not all the verts will be used possibly
    oldToNew.clear();
    // note: this is the index into the array of indices
    int endIndex = (int)indices.size();
    for(int index = 0; index < endIndex; ++index) {
      int oldIndex = indices[index];
      int newIndex;
      auto oldToNewIt = oldToNew.find(oldIndex);
      if(oldToNewIt == oldToNew.end()) {
        newIndex = (int)cube.m_verts.size();
        cube.m_verts.emplace_back(tessVerts[oldIndex]);
        cube.m_packVerts.emplace_back(packVerts[oldIndex]);
        oldToNew.insert(std::make_pair(oldIndex, newIndex));
      } else {
        newIndex = oldToNewIt->second;
      }

      cube.m_indices.emplace_back(newIndex);
    }
  }
}

CanonicalCube QuaxolChunk::s_canonicalCubesByDir[RenderBlock::NumDirs];
void QuaxolChunk::BuildCanonicalCubesByDir(float blockSize) {
  Mesh tesseract;
  // essentially relying on implicit ordering assumption here
  tesseract.buildQuaxolTesseract(blockSize);

  //for(int d = 0; d < RenderBlock::NumDirCombinations; ++d) {
  for(int d = 0; d < RenderBlock::NumDirs; ++d) {
    CanonicalCube& cube = s_canonicalCubesByDir[d];
    cube.m_dirIndex = (RenderBlock::DirIndex)d;
    cube.m_dir = (RenderBlock::Dir)(1 << d);
    cube.m_indices.resize(0);
    cube.m_verts.resize(0);
    const int indicesPerCube = 6 * 2 * 3;
    const int vertsPerCube = 8;
    cube.m_indices.reserve(indicesPerCube); // tri list, bleh
    cube.m_verts.reserve(vertsPerCube);

    int startIndex = d * indicesPerCube;
    int endIndex = (d+1) * indicesPerCube;
    assert(endIndex <= (int)tesseract._indices.size());

    typedef std::map<int, int> OldToNewHash;
    OldToNewHash oldToNew;
    // note: this is the index into the array of indices
    for(int index = startIndex; index < endIndex; ++index) {
      int oldIndex = tesseract._indices[index];
      int newIndex;
      auto oldToNewIt = oldToNew.find(oldIndex);
      if(oldToNewIt == oldToNew.end()) {
        newIndex = (int)cube.m_verts.size();
        cube.m_verts.emplace_back(tesseract._verts[oldIndex]);
        oldToNew.insert(std::make_pair(oldIndex, newIndex));
      } else {
        newIndex = oldToNewIt->second;
      }

      cube.m_indices.emplace_back(newIndex);
    }

    assert((int)cube.m_verts.size() == vertsPerCube);
    assert((int)cube.m_indices.size() == indicesPerCube);
  }
}

void QuaxolChunk::AddRenderCubeByFlag(
    const Vec4f& vertOffset, const QuaxolVert& packOffset, unsigned char connectFlags) {
  CanonicalCube& cube = s_canonicalCubesByFlag[connectFlags];
  assert(cube.m_connectFlags == connectFlags); // call BuildCanonicalCubesByFlag

  int indexOffset = m_verts.size();
  for(auto vert : cube.m_verts) {
    m_verts.emplace_back(vert + vertOffset);
  }
  for(auto vert : cube.m_packVerts) {
    QuaxolVert movedVert = packOffset;
    //as long there isn't overflow this should be fine
    movedVert._position += vert._position;
    m_packVerts.emplace_back(movedVert);
  }
  for(auto index : cube.m_indices) {
    m_indices.emplace_back(index + indexOffset);
  }
}

void QuaxolChunk::AddRenderCubeByDir(
    const Vec4f& vertOffset, RenderBlock::DirIndex dirIndex) {
  CanonicalCube& cube = s_canonicalCubesByDir[dirIndex];
  assert(cube.m_dirIndex == dirIndex); // call BuildCanonicalCubesByDir

  int indexOffset = m_verts.size();
  for(auto vert : cube.m_verts) {
    m_verts.emplace_back(vert + vertOffset);
  }
  for(auto index : cube.m_indices) {
    m_indices.emplace_back(index + indexOffset);
  }
}

void QuaxolChunk::UpdateTrisFromConnects() {

  int expectedVerts = m_cubeCount * 8;
  int expectedTris = m_cubeCount * 12; // probably should do quads?
  m_verts.resize(0);
  m_verts.reserve(expectedVerts);
  m_packVerts.resize(0);
  m_packVerts.reserve(expectedVerts);
  m_indices.resize(0);
  m_indices.reserve(expectedTris * 3);

  Vec4f zeroOffset(0,0,0,0);

  QuaxolVert offsetPackVert;

  for (int x = 0; x < c_mxSz; ++x) {
    offsetPackVert._pos_x = x;
    for (int y = 0; y < c_mxSz; ++y) {
      offsetPackVert._pos_y = y;
      for (int z = 0; z < c_mxSz; ++z) {
        offsetPackVert._pos_z = z;
        for (int w = 0; w < c_mxSz; ++w) {
          offsetPackVert._pos_w = w;

          const Block& block = m_blocks[x][y][z][w];
          const RenderBlock& rBlock = m_connects[x][y][z][w];
          QuaxolSpec blockSpec(x, y, z, w);

          offsetPackVert._uvInd = block.type;
          Vec4f blockCoords = blockSpec.ToFloatCoords(zeroOffset, m_blockSize);
          AddRenderCubeByFlag(blockCoords, offsetPackVert, rBlock.connectFlags);

          //for(int c = 0; c < RenderBlock::NumDirs; ++c) {
          //  if(rBlock.connectFlags & (1 << c)) {
          //    AddRenderCubeByDir(blockCoords, (RenderBlock::DirIndex)c);
          //  }
          //} // dir

        } // w
      } // z
    } // y
  } // x
}

void QuaxolChunk::DebugSwapAxis(int sourceInd, int destInd) {
  Block copyBlocks[c_mxSz][c_mxSz][c_mxSz][c_mxSz];
  memcpy(&copyBlocks, m_blocks, sizeof(m_blocks));

  for (int x = 0; x < c_mxSz; ++x) {
    for (int y = 0; y < c_mxSz; ++y) {
      for (int z = 0; z < c_mxSz; ++z) {
        for (int w = 0; w < c_mxSz; ++w) {
          QuaxolSpec orig(x, y, z, w);
          QuaxolSpec swapped(orig);
          swapped[sourceInd] = orig[destInd];
          swapped[destInd] = orig[sourceInd];

          m_blocks[swapped.x][swapped.y][swapped.z][swapped.w] =
              copyBlocks[orig.x][orig.y][orig.z][orig.w];
        } // w
      } // z
    } // y
  } // x

  UpdateRendering();
}

}; // namespace fd
