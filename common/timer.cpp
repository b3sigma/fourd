#include "timer.h"
#include <assert.h>

namespace fd {

#ifdef WIN32
  double fd::Timer::_frequency = 1.0;
#else //linux
  double fd::Timer::_invFrequency = 1.0 / 1000000000.0;
#endif // WIN32 

bool Timer::RunTests() {
  Timer test1;
  test1.Start();
  double elapsedTime = test1.GetElapsed();
  assert(elapsedTime > 0.0);
  return true;
}

} //namespace fd
