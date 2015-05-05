#include <memory>

#include "quaxol.h"

#include "chunkloader.h"
#include "fourmath.h"

namespace fd {

QuaxolChunk::QuaxolChunk() {
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

    Block& block = m_blocks[local.x][local.y][local.z][local.w];
    block.present = true;
  }

  return true;
}

}; // namespace fd