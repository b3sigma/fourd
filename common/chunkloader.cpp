#include "chunkloader.h"

#include <stdio.h>

namespace fd {

  bool ChunkLoader::LoadFromFile(const char* filename) {
    quadxols_.resize(0);

    FILE* hFile;
    if (0 != fopen_s(&hFile, filename, "rt"))
      return false;

    Quadxol q;

    while (4 == fscanf_s(hFile, "%d %d %d %d\n", &q.x, &q.y, &q.z, &q.w)) {
      quadxols_.push_back(q);
    }

    fclose(hFile);

    return true;
  }

} // namespace fd