#pragma once

#include <vector>
#include <tuple>
#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#include "fourmath.h"
#include "camera.h"
#include "timer.h"

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

  // shouldn't be here..
  // should be in a scene or something
  ::fd::Timer timer_;
  double _lastTotalTime;
  double _frameTime;


public:
  Render() : _frameTime(0.0), _lastTotalTime(0.0) {}

  void Step() {
    double totalTime = GetTotalTime();
    _frameTime = totalTime - _lastTotalTime;
    _lastTotalTime = totalTime;
  }

  double GetFrameTime() {
    return _frameTime;
  }

  double GetTotalTime() {
    return timer_.GetElapsed();
  }


};

}  // namespace fd
