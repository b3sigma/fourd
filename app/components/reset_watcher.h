#pragma once

#include "../../common/camera.h"
#include "../../common/component.h"
#include "../../common/fourmath.h"
#include "screensaver.h"

namespace fd {

// the premise is to wait for any input signal
// any input resets the clock
// if the clock expires, we want to reset the whole state and start off the screensaver
// the screensaver kills itself if it receives any input signal, otherwise it spins the view around

// this can probably be refactored into a timer like a signal watcher
// which we will do the second time we need this (memory failure notwithstanding)
class ResetWatcherComponent : public Component {
public:
  // manually tracking frame time seems redundant, 
  // but also sort of nice from separation of components
  float m_lastInputTime;
  float m_trackedFrameTime;
  float m_timeout;

public:
  ResetWatcherComponent(float timeout) 
      : m_trackedFrameTime(0.0f)
      , m_lastInputTime(0.0f) 
      , m_timeout(timeout) {
    assert(timeout > 0.0f);
  }

  virtual void OnConnected() {
    RegisterSignal(std::string("Step"), this, &ResetWatcherComponent::OnStepSignal);
    RegisterSignal(std::string("AnyInput"), this, &ResetWatcherComponent::OnAnyInput);    
  }

  void OnAnyInput() {
    m_lastInputTime = m_trackedFrameTime;
  }

  void OnStepSignal(float delta) {
    m_trackedFrameTime += delta;

    if(m_trackedFrameTime - m_lastInputTime > m_timeout) {
      m_lastInputTime = m_trackedFrameTime;
      m_ownerBus->SendSignal(std::string("AnyInput"), SignalN<>());


      m_ownerBus->AddComponent(new ScreenSaverComponent());
    }
  }
};

}; //namespace fd
