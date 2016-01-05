#pragma once

#include "../../common/component.h"

#include "../imgui_wrapper.h"

namespace fd {

class GuiInputRouterComponent : public Component {

public:
  GuiInputRouterComponent()  {}
  
  virtual void OnConnected() {
    RegisterSignal(std::string("inputControlsMenu"), this, &GuiInputRouterComponent::OnControlsMenu);
    RegisterSignal(std::string("inputMainMenu"), this, &GuiInputRouterComponent::OnMainMenu);
    
  }

  // because I suck, all input callbacks take a frametime
  void OnControlsMenu(float frameTime) {
    ImGuiWrapper::ToggleControllerMenu();
  }

  void OnMainMenu(float frameTime) {
    //ImGuiWrapper::ToggleControllerMenu();
  }
};

}; //namespace fd
