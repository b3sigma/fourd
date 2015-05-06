#pragma once

#include "quaxol.h"

namespace fd {

  class ChunkLoader {
    
  public:
    ChunkLoader() {}

    bool LoadFromFile(const char* filename);

    TVecQuaxol quaxols_;
  };
}