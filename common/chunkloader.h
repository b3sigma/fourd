#pragma once

#include <vector>

namespace fd {

  // This is going to immediately suck from a space perspective.
  // Should at minimal do smaller sizes and then chunks hold some position.
  // Actually, this isn't even a quadxol, you liar, this is just a shitty
  // block list.
  // But today, just code.
  // Save the pain of doing things right for tomorrow-me. Damn you, past-me,
  // for instilling these bad habits!
  class Quadxol {
  public:
    union {
      int p[4];
      struct {
        int x, y, z, w;
      };
    };
  };
  typedef std::vector<Quadxol> TVecQuadxol;

  class ChunkLoader {
    
  public:
    ChunkLoader() {}

    bool LoadFromFile(const char* filename);

    TVecQuadxol quadxols_;
  };
}