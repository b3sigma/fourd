#include "chunkloader.h"

#include <stdio.h>
#include <memory>

#include "fd_simple_file.h"
#include "filedata.h"

namespace fd {

  QuaxolChunk* ChunkLoader::LoadFromTextFile(const char* filename) {
#pragma warning(push)
#pragma warning(disable: 4996) //strerror whining
    FILE* hFile;
    errno_t err;
    if (0 != (err = fopen_s(&hFile, filename, "rt"))) {
      printf("Opening %s failed with err:%s", filename, strerror(err));
      return false;
    }
#pragma warning(pop)

    TVecQuaxol quaxols;
    QuaxolSpec q;

    // Note: before you do anything more complicated than this,
    // check out capnproto. Probably less efficient than direct binary
    // fread and such, but probably much more flexible and versioning resistant.
    while (4 == fscanf_s(hFile, "%d %d %d %d\n", &q.x, &q.y, &q.z, &q.w)) {
      quaxols.push_back(q);
    }
    fclose(hFile);

    Vec4f offset;
    QuaxolSpec gridOffset(offset);
    QuaxolChunk* chunk = new QuaxolChunk(offset, Vec4f(10.0f, 10.0f, 10.0f, 10.0f));
    chunk->LoadFromList(&quaxols, &gridOffset);
    return chunk;
  }

  QuaxolChunk* ChunkLoader::LoadFromFile(const char* filename) {
  
    if(strstr(filename, ".txt")) {
      return LoadFromTextFile(filename);
    } else {
      std::unique_ptr<FileData> file(FileData::LoadFromFile(filename));
      if(!file.get())
        return NULL;

      return LoadFromFileData(file.get());
    }
  }

  QuaxolChunk* ChunkLoader::LoadFromFileData(FileData* file) {
    int32_t fileHeader;
    file->read(fileHeader);
    if(fileHeader != c_headerSignature)
      return NULL;

    int32_t version;
    file->read(version);

    Vec4f position;
    file->read(position);
    Vec4f blockSize;
    file->read(blockSize);

    std::unique_ptr<QuaxolChunk> chunk(
        new QuaxolChunk(position, blockSize));

    assert(sizeof(Block) == 2);

    // to future me who reordered or added to blocks, hahah!
    file->readRaw((unsigned char*)&(chunk->m_blocks), sizeof(chunk->m_blocks));

    return chunk.release();
  }

  bool ChunkLoader::SaveToFile(const char* filename, const QuaxolChunk* chunk) {
    std::unique_ptr<FileData> file(FileData::OpenForWriting(filename));

    file->write(c_headerSignature);
    file->write(c_version);
    file->write(chunk->m_position);
    file->write(chunk->m_blockSize);
    file->writeRaw((unsigned char*)&chunk->m_blocks, sizeof(chunk->m_blocks));
    return file->SaveToFile();
  }

} // namespace fd