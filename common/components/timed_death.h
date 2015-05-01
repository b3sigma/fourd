#pragma once

#pragma once

#include <algorithm>

#include "../component.h"

namespace fd {

class TimedDeath : public Component {
protected:
  float _duration;
  float _current;

public:
  TimedDeath(float duration)
      : _duration(duration)
      , _current(0.0f)
  {
    assert(duration != 0.0f);
  }

  virtual ~TimedDeath() {
    assert(_duration != 0.0f); // just need a line for the debugger
  }

  virtual void OnConnected() {
    RegisterSignal(std::string("Step"), this, &TimedDeath::OnStepSignal);
  }

  void OnStepSignal(float delta) {
    _current += delta;
    if(_current >= _duration) {
      SelfDestruct();
      m_ownerBus->SendSignal(std::string("DeleteSelf"), SignalN<>());
    }
  }
};

} // namespace fd
