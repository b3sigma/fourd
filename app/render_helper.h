#pragma once

#include "../common/fourmath.h"

namespace fd {

class RenderHelper {
public:
  enum EyeCandyTypes {
    EyeCandyQuad,
    EyeCandyCube,
    EyeCandyTesseract,
    EyeCandy16Cell,
    EyeCandy24Cell,
    EyeCandy120Cell,
    EyeCandy600Cell,
  };

  static void RenderTess(Vec4f pos, const Mat4f* rotation = NULL);
  static void AddEyeCandy(EyeCandyTypes type, const Vec4f& pos);
};

} //namespace fd
