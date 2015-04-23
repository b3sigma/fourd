#pragma once

#include <vector>

namespace fd {

  // This is going to immediately suck from a space perspective.
  // Should at minimal do smaller sizes and then chunks hold some position.
  // Actually, this isn't even a quaxol, you liar, this is just a shitty
  // block list.
  // But today, just code.
  // Save the pain of doing things right for tomorrow-me. Damn you, past-me,
  // for instilling these bad habits!
  class Quaxol {
  public:
    union {
      int p[4];
      struct {
        int x, y, z, w;
      };
    };
  };
  typedef std::vector<Quaxol> TVecQuaxol;

  class ChunkLoader {
    
  public:
    ChunkLoader() {}

    bool LoadFromFile(const char* filename);

    TVecQuaxol quaxols_;
  };
}