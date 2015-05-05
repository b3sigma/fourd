#pragma once

#include "chunkloader.h"
#include "fourmath.h"

namespace fd {
 
  struct Block {
    // bit pack this shortly, but let's get it running first
    bool present;
  };
  
  const int c_mxSz = 16; // this means 16^4 blocks, or 32k
  // should be indexed [x][y][z][w]?
  // would imply cache locality pulls up w first

  class QuaxolChunk {
  public:
    Block m_blocks[c_mxSz][c_mxSz][c_mxSz][c_mxSz];
  
  public:
    QuaxolChunk();
    ~QuaxolChunk();

    bool LoadFromList(const TVecQuaxol* pPresent, const QuaxolSpec* offset);
  };

}; // namespace fd