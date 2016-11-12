#pragma once

#include "../common/fourmath.h"

namespace fd {

class Entity;

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
    EyeCandySpherinder,
  };

  static void AddEyeCandy(EyeCandyTypes type, const Vec4f& pos);

  static Entity* RenderTess(Vec4f pos, const Mat4f* rotation = NULL, Vec4f color = Vec4f::s_ones, float scale = 1.0f);
  static void RenderAxis(Vec4f pos, const Mat4f* rotation = NULL, float scale = 20.0f, bool permanent = true);
  static void SpamAxes(Vec4f pos);
};

} //namespace fd
