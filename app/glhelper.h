#pragma once

#include <stdio.h>
#include <GL/glew.h>

//#ifdef _DEBUG
inline bool WasGLErrorPlusPrint() {
  static bool succeededOnce = false;
  GLenum errCode = glGetError();
  const GLubyte *errString;
  if(!succeededOnce && errCode == GL_INVALID_OPERATION) {
    return false; // calling glGetError too soon is an error?
  } else {
    // once it succeeds, treat behavior normally
    succeededOnce = true;
  }

  if (errCode != GL_NO_ERROR) {
    errString = gluErrorString(errCode);
    printf("OpenGL Error: %s\n", errString);
    return true;
  } else {
    return false;
  }
}

inline bool WasGLErrorPlusPrintSimple() {
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

// only valid when framebuffers are bound maybe?
inline bool WasFramebufferError() {
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    printf("Framebuffer error: %d!\n", status);
    return true;
  }
  return false;
}

inline bool WasGLUErr(int gluErr) {
  if(gluErr != 0) {
    printf("build mips error:%s\n", gluErrorString(gluErr));
    return false;
  } else {
    return true;
  }
}
//#else //_DEBUG
//inline bool WasGLErrorPlusPrint() { return false; }
//inline bool WasGLUErr(int gluErr) { return (gluErr == 0); }
//#endif //_DEBUG
