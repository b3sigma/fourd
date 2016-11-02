#include "fourmath.h"

namespace fd {

Vec4f Vec4f::s_ones = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);
Mat4f Mat4f::s_ident = Mat4f().storeIdentity();

} //namespace fd
