#pragma once

#ifdef WIN32
#include <windows.h>
#endif // WIN32

#include <string>

namespace fd {

  class Timer
  {
  protected:
    std::string m_message;
    double _elapsed;
#ifdef WIN32
    LARGE_INTEGER _start;
    static double _frequency;
#else
#pragma error "TODO: portable timer class"
#endif

  public:
    Timer(const std::string& message) : Timer() {
      m_message = message;
    }

    Timer()
    {
      if (_frequency == 1.0) {
        LARGE_INTEGER frequency;
        frequency.QuadPart = 2;
        // TODO: handle systems where there is no qpc
        QueryPerformanceFrequency(&frequency);
        _frequency = static_cast<double>(frequency.QuadPart);
      }
      Start();
    }

    ~Timer() {
      if(!m_message.empty()) {
        printf("Timer '%s' expired after %f seconds\n", m_message.c_str(), GetElapsed());
      }
    }

    void Start() {
      QueryPerformanceCounter(&_start);
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


