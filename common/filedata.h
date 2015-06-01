#pragma once

#include <vector>

namespace fd {
  
  class FileData {
  public:
    unsigned char* m_data;
    size_t m_dataSize;

    size_t m_currentRead;

  public:
    FileData() : m_data(NULL), m_dataSize(0), m_currentRead(0) {}
    ~FileData() { delete m_data; }

    static FileData* LoadFromFile(const char* filename);
    
    bool WriteToFile(const char* filename);

    template <typename T>
    bool read(T& val) {

      return false;
    }

  };

}; //namespace fd