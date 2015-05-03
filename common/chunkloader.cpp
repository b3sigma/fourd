#include "chunkloader.h"

#include <stdio.h>

namespace fd {

  bool ChunkLoader::LoadFromFile(const char* filename) {
    quaxols_.resize(0);

#pragma warning(push)
#pragma warning(disable: 4996)
    FILE* hFile;
    errno_t err;
    if (0 != (err = fopen_s(&hFile, filename, "rt"))) {
      printf("Opening %s failed with err:%s", filename, strerror(err));
      return false;
    }
#pragma warning(pop)

    Quaxol q;

    while (4 == fscanf_s(hFile, "%d %d %d %d\n", &q.x, &q.y, &q.z, &q.w)) {
      quaxols_.push_back(q);
    }

    fclose(hFile);

    return true;
  }

} // namespace fd