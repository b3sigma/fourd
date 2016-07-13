#include <memory>
#include <stdio.h>
#include <string.h>

#include "filedata.h"
#include "fd_simple_file.h"


namespace fd {

  FileData* FileData::LoadFromFile(const char* filename) {
    std::unique_ptr<FileData> file(new FileData);

    if(!fd_file_to_vec(filename, file->m_ownedData)) {
      return NULL;
    }
    file->m_raw = &(file->m_ownedData[0]);
    file->m_dataSize = file->m_ownedData.size();
    file->m_filename.assign(filename);

    return file.release();
  }

  FileData* FileData::OpenForWriting(const char* filename) {
    std::unique_ptr<FileData> file(new FileData);
    file->m_filename.assign(filename);

    return file.release();
  }

  bool FileData::SaveToFile() {
    return fd_file_write_vec(m_filename.c_str(), m_ownedData);
  }

}; //namespace fd
