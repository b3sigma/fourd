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
  memset(&m_blocks, 0, sizeof(m_blocks));

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

    Block& block = GetBlock(local.x, local.y, local.z, local.w);
    block.present = true;
  }

  UpdateConnects();
  UpdateTrisFromConnects();

  return true;
}
 
void QuaxolChunk::UpdateConnects() {
  // do this the slow but simple way as a first pass
  // index x,y,w,z currently, so do that loop
  m_cubeCount = 0;
  for (int x = 0; x < c_mxSz; ++x) {
    for (int y = 0; y < c_mxSz; ++y) {
      for (int w = 0; w < c_mxSz; ++w) {
        for (int z = 0; z < c_mxSz; ++z) {
          RenderBlock& rBlock = m_connects[x][y][w][z];
          rBlock.connectFlags = 0;
          if (!IsPresent(x, y, z, w)) {
            continue;
          }

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

CanonicalCube QuaxolChunk::s_canonicalCubes[RenderBlock::NumDirs];
void QuaxolChunk::BuildCanonicalCubes(float blockSize) {
  Mesh tesseract;
  // essentially relying on implicit ordering assumption here
  tesseract.buildQuaxolTesseract(blockSize);

  for(int d = 0; d < RenderBlock::NumDirs; ++d) {
    CanonicalCube& cube = s_canonicalCubes[d];
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

void QuaxolChunk::AddRenderCube(
    const Vec4f& vertOffset, RenderBlock::DirIndex dirIndex) {
  CanonicalCube& cube = s_canonicalCubes[dirIndex];
  assert(cube.m_dirIndex == dirIndex); // call BuildCanonicalCubes

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
  m_indices.resize(0);
  m_indices.reserve(expectedTris * 3);

  Vec4f zeroOffset(0,0,0,0);

  for (int x = 0; x < c_mxSz; ++x) {
    for (int y = 0; y < c_mxSz; ++y) {
      for (int w = 0; w < c_mxSz; ++w) {
        for (int z = 0; z < c_mxSz; ++z) {
          const RenderBlock& rBlock = m_connects[x][y][w][z];
          QuaxolSpec blockSpec(x, y, z, w);
          // I guess do offset at render time
          // Begs the question of whether to do scaling also
          Vec4f blockCoords = blockSpec.ToFloatCoords(zeroOffset, m_blockSize);
          for(int c = 0; c < RenderBlock::NumDirs; ++c) {
            if(rBlock.connectFlags & (1 << c)) {
              AddRenderCube(blockCoords, (RenderBlock::DirIndex)c);
            }
          } // dir

        } // z
      } // w
    } // y
  } // x
}

}; // namespace fd