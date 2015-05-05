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
  class QuaxolSpec {
  public:
    union {
      int p[4];
      struct {
        int x, y, z, w;
      };
    };
    QuaxolSpec() : x(0), y(0), z(0), w(0) {}
    QuaxolSpec(int inX, int inY, int inZ, int inW) : x(inX), y(inY), z(inZ), w(inW) {} 
    QuaxolSpec(const QuaxolSpec& copy) : x(copy.x), y(copy.y), z(copy.z), w(copy.w) {}
    QuaxolSpec& operator -= (const QuaxolSpec& r) {
      x -= r.x; y -= r.y; z -= r.z; w -= r.w;
      return *this;
    }
  };
  typedef std::vector<QuaxolSpec> TVecQuaxol;

  class ChunkLoader {
    
  public:
    ChunkLoader() {}

    bool LoadFromFile(const char* filename);

    TVecQuaxol quaxols_;
  };
}