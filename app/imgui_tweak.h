#pragma once

#include "../common/fourmath.h"

namespace fd {

class TweakWindow {
public:
  static void RenderWindow(float frameTime, const Vec2f& offset);
};

}  //namespace fd