#include "frame_timer.h"

#include <vector>

#include "timer.h"

namespace fd {

// next add the dedup when the same scope is pushed and poped multiple times
// then a real tree structure that doesn't alloc memory beyond the first frame

typedef std::vector<Timer> ScopeStack;
ScopeStack g_timers;

void FrameTimer::PushScope(const char* scope) {
  g_timers.emplace_back(Timer(scope));
}
 
void FrameTimer::PopScope(const char* scope) {
  g_timers.pop_back();
}

} //namespace fd
