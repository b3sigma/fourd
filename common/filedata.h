#pragma once

#include <vector>

namespace fd {
  
  class FileData {
  public:
    typedef std::vector<unsigned char> DataVec;
    DataVec m_ownedData; // for writing

    const unsigned char* m_raw; // for reads, not owned mem
    size_t m_dataSize;

    size_t m_currentRead;
    std::string m_filename;

  public:
    FileData() : m_raw(NULL), m_dataSize(0), m_currentRead(0) {}
    FileData(unsigned char* data, size_t dataSize) 
        : m_raw(data), m_dataSize(dataSize), m_currentRead(0) {}
    ~FileData() { }

    static FileData* LoadFromFile(const char* filename);
    static FileData* OpenForWriting(const char* filename); // doesn't do any disk activity until save

    bool SaveToFile();

    template <typename T>
    bool read(T& val) {
      //ntol stuff?
      size_t size = sizeof(val);
      if(m_currentRead + size > m_dataSize)
        return false;
      val = *(T*)(&m_raw[m_currentRead]);
      m_currentRead += size;
      
      return true;
    }

    bool readRaw(unsigned char* buffer, size_t size) {
      if(m_currentRead + size > m_dataSize)
        return false;
      memcpy(buffer, &m_raw[m_currentRead], size);
      m_currentRead += size;
      return true;
    }

    template <class TT> struct ConstRemover { typedef TT typeval; };
    template <class TT> struct ConstRemover<const TT> { typedef TT typeval; };
    
    template <typename T>
    void write(T& val) {
      size_t size = sizeof(val);
      size_t current = m_ownedData.size();
      m_ownedData.resize(current + size);
      unsigned char* dataPtr = &m_ownedData[current];
      auto typedPtr = const_cast<ConstRemover<T>::typeval *>((T*)dataPtr);
      *typedPtr = val;
    }

    void writeRaw(unsigned char* buffer, size_t size) {
      size_t current = m_ownedData.size();
      m_ownedData.resize(current + size);
      memcpy(&m_ownedData[current], buffer, size);
    }

  };

}; //namespace fd