#pragma once

#include <vector>
#include <tuple>
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#include <GL/gl.h>
#include <GL/glu.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <GL/glut.h>
//#include <GL/freeglut.h>

#include "fourmath.h"
#include "camera.h"

namespace fd {

// Render should contain all the GL code
// View will contain a specific render target and a camera

class Input {
  // Wow this is so useful. Good thing it's here.
};

/*
  The view is abstracted from the camera as it may be useful
  to render the same camera and scene onto multiple render targets.
  Many of the fields could plausibly go in either structure.
  */
class View {
  class Viewport {
    int _left;
    int _right;
    int _top;
    int _bottom;
  };
  float _near;
  float _far;
  float _fov;
  Camera _camera;
};


class Render {
  typedef std::vector<View*> TViewList;
  TViewList _views;
};

}  // namespace fd
