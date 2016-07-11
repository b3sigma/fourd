#include "timer.h"

#ifdef WIN32
  double fd::Timer::_frequency = 1.0;
#else //linux
  double fd::Timer::_invFrequency = 1.0 / 1000000000.0;
#endif // WIN32
