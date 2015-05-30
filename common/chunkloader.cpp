#include "chunkloader.h"

#include <stdio.h>
#include <memory>

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

  bool ChunkLoader::LoadFromTextFile(const char* filename) {
#pragma warning(push)
#pragma warning(disable: 4996)
    FILE* hFile;
    errno_t err;
    if (0 != (err = fopen_s(&hFile, filename, "rt"))) {
      printf("Opening %s failed with err:%s", filename, strerror(err));
      return false;
    }
#pragma warning(pop)

    QuaxolSpec q;

    // Note: before you do anything more complicated than this,
    // check out capnproto. Probably less efficient than direct binary
    // fread and such, but probably much more flexible and versioning resistant.
    while (4 == fscanf_s(hFile, "%d %d %d %d\n", &q.x, &q.y, &q.z, &q.w)) {
      quaxols_.push_back(q);
    }

    fclose(hFile);
    return true;
  }

  bool ChunkLoader::LoadFromFile(const char* filename) {
    quaxols_.resize(0);

    if(strstr(filename, ".txt")) {
      return LoadFromTextFile(filename);
    } else {
      return false;
    }

    return true;
  }

} // namespace fd