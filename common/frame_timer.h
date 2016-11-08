#pragma once

namespace fd {

class FrameTimer {
public:
  static void NewFrame();

  static void PushScope(const char* scope);
  static void PopScope(const char* scope);
};

class FrameScoper {
  const char* _scope;
public:
  // only good for statically defined strings where we can use the pointer address to hash
  FrameScoper(const char* scope) : _scope(scope) {
    FrameTimer::PushScope(_scope);
  }
  ~FrameScoper() {
    FrameTimer::PopScope(_scope);
  }
};

#define SCOPE_TIME() FrameScoper stackFrameScoper##__LINE__(__FUNCTION__);
#define SCOPE_TIME_NAMED(str) FrameScoper stackFrameScoper##__LINE__(str);

} //namespace fd