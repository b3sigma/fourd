#pragma once

#include "quaxol.h"
#include "stdint.h"

namespace fd {

  class FileData;
  class QuaxolChunk;

  class ChunkLoader {
    const int32_t c_headerSignature = 0xdeadb00b;
    const int32_t c_version = 1;
    
  public:
    ChunkLoader() {}

    bool SaveToFile(const char* filename, const QuaxolChunk* chunk);
    QuaxolChunk* LoadFromFile(const char* filename);
    QuaxolChunk* LoadFromTextFile(const char* filename);
    QuaxolChunk* LoadFromFileData(FileData* file);

  };
}