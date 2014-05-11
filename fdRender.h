#pragma once

#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <GL/glut.h>

#include "fdCamera.h"

// Render should contain all the GL code
// View will contain a specific render target and a camera

class Input {
  // TODO: actually use any of this.
};

class View {
  Camera _camera;
};


class Render {
  typedef std::vector<View*> TViewList;
  TViewList _views;
};
