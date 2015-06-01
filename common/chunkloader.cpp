#include "chunkloader.h"

#include <stdio.h>
#include <memory>

#include "fd_simple_file.h"
#include "filedata.h"

namespace fd {

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
      std::unique_ptr<FileData> file(FileData::LoadFromFile(filename));
      if(!file.get())
        return false;

      if(!LoadFromFileData(file.get()))
        return false;

      return true;
    }

    return true;
  }

  bool ChunkLoader::LoadFromFileData(const FileData* file) {
    QuaxolChunk* chunk = new QuaxolChunk(Vec4f(), Vec4f(10.0f, 10.0f, 10.0f, 10.0f));

    const int fileHeader = 0xdeadb00b;

    return false;
  }

} // namespace fd