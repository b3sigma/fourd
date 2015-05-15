#pragma once

#include <stdio.h>
#include <gl/glew.h>

#ifdef _DEBUG
inline bool WasGLErrorPlusPrint() {
  GLenum errCode;
  const GLubyte *errString;

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    errString = gluErrorString(errCode);
    printf("OpenGL Error: %s\n", errString);
    return true;
  } else {
    return false;
  }
}

inline bool WasGLUErr(int gluErr) {
  if(gluErr != 0) {
    printf("build mips error:%s\n", gluErrorString(gluErr));
    return false;
  } else {
    return true;
  }
}
#else //_DEBUG
inline bool WasGLErrorPlusPrint() { return false; }
inline bool WasGLUErr(int gluErr) { return (gluErr != 0); }
#endif //_DEBUG