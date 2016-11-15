#pragma once

#ifdef WIN32
#include <windows.h>
#else //linux
#include <time.h>
#endif // os

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
    timespec _start;
    static double _invFrequency;
#endif

  public:
#ifdef WIN32
    typedef LARGE_INTEGER TimerPrimative;
#else
    typedef timespec TimerPrimative;
#endif

  public:
    Timer(const std::string& message) : Timer() {
      m_message = message;
    }

    Timer(Timer& rCopy)
        : m_message(rCopy.m_message)
        , _elapsed(rCopy._elapsed)
        , _start(rCopy._start)
    {
      rCopy.m_message.clear(); // cuz destructor message... ugh
    }

    Timer() : _elapsed(0.0)
    {
      #ifdef WIN32
        if (_frequency == 1.0) {
          LARGE_INTEGER frequency;
          frequency.QuadPart = 2;
          // TODO: handle systems where there is no qpc
          QueryPerformanceFrequency(&frequency);
          _frequency = static_cast<double>(frequency.QuadPart);
        }
      #endif //WIN32
      Start();
    }

    ~Timer() {
      if(!m_message.empty()) {
        printf("Timer '%s' expired after %f seconds\n", m_message.c_str(), GetElapsed());
      }
    }

    void Start() {
      #ifdef WIN32
        QueryPerformanceCounter(&_start);
      #else //linux
        clock_gettime(CLOCK_MONOTONIC_RAW, &_start);
      #endif //os
    }

    double GetElapsed() {
      #ifdef WIN32
        LARGE_INTEGER current;
        if (QueryPerformanceCounter(&current)) {
          _elapsed = static_cast<double>(current.QuadPart - _start.QuadPart) / _frequency;
        }
      #else // linux
        timespec current;
        clock_gettime(CLOCK_MONOTONIC_RAW, &current);
        _elapsed = static_cast<double>(current.tv_sec - _start.tv_sec);
        _elapsed += static_cast<double>(current.tv_nsec - _start.tv_nsec) * _invFrequency;
      #endif //os
      return _elapsed;
    }

    static bool RunTests();
  };

};
