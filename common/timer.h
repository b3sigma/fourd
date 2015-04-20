#pragma once

#ifdef WIN32
#include <windows.h>
#endif // WIN32

namespace fd {

  class Timer
  {
  protected:
    double _elapsed;
#ifdef WIN32
    LARGE_INTEGER _start;
    static double _frequency;
#else
#pragma error "TODO: portable timer class"
#endif

  public:
    Timer()
    {
      _start.QuadPart = 0;
      if (_frequency == 1.0) {
        LARGE_INTEGER frequency;
        frequency.QuadPart = 2;
        // TODO: handle systems where there is no qpc
        QueryPerformanceFrequency(&frequency);
        _frequency = static_cast<double>(frequency.QuadPart);
      }
    }

    double GetElapsed() {
      LARGE_INTEGER current;
      if (SUCCEEDED(QueryPerformanceCounter(&current))) {
        _elapsed = static_cast<double>(current.QuadPart - _start.QuadPart) / _frequency;
      }

      return _elapsed;
    }
  };

};


