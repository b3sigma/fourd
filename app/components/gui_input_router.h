#pragma once

#include "../../common/component.h"

#include "../imgui_wrapper.h"

namespace fd {

class GuiInputRouterComponent : public Component {

  float m_lastResetMenuTime;
  float m_trackedFrameTime;

public:
  GuiInputRouterComponent() 
      : m_trackedFrameTime(0.0f)
      , m_lastResetMenuTime(0.0f)
  {}
  
  virtual void OnConnected() {
    RegisterSignal(std::string("inputControlsMenu"), this, &GuiInputRouterComponent::OnControlsMenu);
    RegisterSignal(std::string("inputMainMenu"), this, &GuiInputRouterComponent::OnReset);
    RegisterSignal(std::string("Step"), this, &GuiInputRouterComponent::OnStep);
    
  }

  void OnStep(float delta) {
    m_trackedFrameTime += delta;
  }

  // because I suck, all input callbacks take a frametime
  void OnControlsMenu(float frameTime) {
    ImGuiWrapper::ToggleControllerMenu();
  }

  void OnReset(float frameTime) {
    bool showingMenu = ImGuiWrapper::ToggleResetMenu();

    if(showingMenu) {
      const float resetTime = 1.5f;
      float delta = m_trackedFrameTime - m_lastResetMenuTime;
      if(delta < resetTime) {
        m_ownerBus->SendSignal(std::string("RestartGameState"), SignalN<>());
      }
      m_lastResetMenuTime = m_trackedFrameTime;
    }
  }
};

}; //namespace fd
