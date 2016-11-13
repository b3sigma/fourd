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
  
  virtual void OnConnected();

  // need named signals so callbacks can get the signal type?
  // then just a string to whatever to go to imgui
  void MapSignalToImguiType(const std::string& signal);
  void OnPadUp(float frameTime);
  void OnPadUp_Release(float frameTime);
  void OnPadDown(float frameTime);
  void OnPadDown_Release(float frameTime);
  void OnPadLeft(float frameTime);
  void OnPadLeft_Release(float frameTime);
  void OnPadRight(float frameTime);
  void OnPadRight_Release(float frameTime);
  void OnButton0(float frameTime);
  void OnButton0_Release(float frameTime);
  void OnButton1(float frameTime);
  void OnButton1_Release(float frameTime);

  void OnStep(float delta);

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
