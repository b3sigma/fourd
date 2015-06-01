#include "filedata.h"

#include <memory>
#include <stdio.h>
#include "fd_simple_file.h"

namespace fd {

  FileData* FileData::LoadFromFile(const char* filename) {
    std::unique_ptr<FileData> file(new FileData);

    unsigned char* data = NULL;
    size_t dataSize;
    if(!fd_file_to_byte(filename, &data, dataSize)) {
      return NULL;
    }

    file->m_data = data;
    file->m_dataSize = dataSize;
    return file.release();
  }

}; //namespace fd
