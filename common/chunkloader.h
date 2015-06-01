#pragma once

#include "quaxol.h"

namespace fd {

  class FileData;

  class ChunkLoader {
    
  public:
    ChunkLoader() {}

    bool LoadFromFile(const char* filename);
    bool LoadFromTextFile(const char* filename);
    bool LoadFromFileData(const FileData* file);

    TVecQuaxol quaxols_;
  };
}