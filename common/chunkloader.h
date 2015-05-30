#pragma once

#include "quaxol.h"

namespace fd {
  class FileData {
  public:
    unsigned char* m_data;
    size_t m_dataSize;

    FileData() : m_data(NULL), m_dataSize(NULL) {}
    ~FileData() { delete m_data; }

    static FileData* LoadFromFile(const char* filename);

  };


  class ChunkLoader {
    
  public:
    ChunkLoader() {}

    bool LoadFromFile(const char* filename);
    bool LoadFromTextFile(const char* filename);

    TVecQuaxol quaxols_;
  };
}