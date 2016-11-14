#include "gui_input_router.h"

#include <functional>
#include <unordered_map>

#include "../imgui/imgui.h"

namespace fd {

  
void SetNavTypeFromSignal(const std::string& signal, float val) {
  typedef std::unordered_map<std::string, int> CommandHash;
  static CommandHash commands;
  if(commands.empty()) {
    // ok this ended up being a ton of plumbing and we still don't have configurable keybindings yet...
    commands.insert(std::make_pair(std::string("inputPadUp"), ImGuiNavInput_PadUp));
    commands.insert(std::make_pair(std::string("inputPadUp_Release"), ImGuiNavInput_PadUp));
    commands.insert(std::make_pair(std::string("inputPadDown"), ImGuiNavInput_PadDown));
    commands.insert(std::make_pair(std::string("inputPadDown_Release"), ImGuiNavInput_PadDown));
    commands.insert(std::make_pair(std::string("inputPadLeft"),         ImGuiNavInput_PadLeft));
    commands.insert(std::make_pair(std::string("inputPadLeft_Release"), ImGuiNavInput_PadLeft));
    commands.insert(std::make_pair(std::string("inputPadRight"),         ImGuiNavInput_PadRight));
    commands.insert(std::make_pair(std::string("inputPadRight_Release"), ImGuiNavInput_PadRight));
    commands.insert(std::make_pair(std::string("inputButton0"),         ImGuiNavInput_PadActivate));
    commands.insert(std::make_pair(std::string("inputButton0_Release"), ImGuiNavInput_PadActivate));
    commands.insert(std::make_pair(std::string("inputButton1"),         ImGuiNavInput_PadCancel));
    commands.insert(std::make_pair(std::string("inputButton1_Release"), ImGuiNavInput_PadCancel));

    //ImGuiNavInput_PadActivate,      // press button, tweak value                    // e.g. Circle button
    //ImGuiNavInput_PadCancel,        // close menu/popup/child, lose selection       // e.g. Cross button
    //ImGuiNavInput_PadInput,         // text input                                   // e.g. Triangle button
    //ImGuiNavInput_PadMenu,          // access menu, focus, move, resize             // e.g. Square button
    //ImGuiNavInput_PadUp,            // move up, resize window (with PadMenu held)   // e.g. D-pad up/down/left/right, analog
    //ImGuiNavInput_PadDown,          // move down
    //ImGuiNavInput_PadLeft,          // move left
    //ImGuiNavInput_PadRight,         // move right
    //ImGuiNavInput_PadScrollUp,      // scroll up, move window (with PadMenu held)   // e.g. right stick up/down/left/right, analog
    //ImGuiNavInput_PadScrollDown,    // "
    //ImGuiNavInput_PadScrollLeft,    //
    //ImGuiNavInput_PadScrollRight,   //
    //ImGuiNavInput_PadFocusPrev,     // next window (with PadMenu held)              // e.g. L-trigger
    //ImGuiNavInput_PadFocusNext,     // prev window (with PadMenu held)              // e.g. R-trigger
    //ImGuiNavInput_PadTweakSlow,     // slower tweaks                                // e.g. L-trigger, analog
    //ImGuiNavInput_PadTweakFast,   
  }

  auto foundCmdIt = commands.find(signal);
  if(foundCmdIt == commands.end())
    return;

  ImGuiIO& io = ImGui::GetIO();
  io.NavInputs[foundCmdIt->second] = val;
}

void GuiInputRouterComponent::OnConnected() {
  RegisterSignal(std::string("inputControlsMenu"), this, &GuiInputRouterComponent::OnControlsMenu);
  RegisterSignal(std::string("inputMainMenu"), this, &GuiInputRouterComponent::OnReset);
  RegisterSignal(std::string("inputPadUp"), this,         &GuiInputRouterComponent::OnPadUp);
  RegisterSignal(std::string("inputPadUp_Release"), this, &GuiInputRouterComponent::OnPadUp_Release);
  RegisterSignal(std::string("inputPadDown"), this,         &GuiInputRouterComponent::OnPadDown);
  RegisterSignal(std::string("inputPadDown_Release"), this, &GuiInputRouterComponent::OnPadDown_Release);
  RegisterSignal(std::string("inputPadLeft"), this,         &GuiInputRouterComponent::OnPadLeft);
  RegisterSignal(std::string("inputPadLeft_Release"), this, &GuiInputRouterComponent::OnPadLeft_Release);
  RegisterSignal(std::string("inputPadRight"), this,          &GuiInputRouterComponent::OnPadRight);
  RegisterSignal(std::string("inputPadRight_Release"), this,  &GuiInputRouterComponent::OnPadRight_Release);
  RegisterSignal(std::string("inputButton0"), this,          &GuiInputRouterComponent::OnButton0);
  RegisterSignal(std::string("inputButton0_Release"), this,  &GuiInputRouterComponent::OnButton0_Release);
  RegisterSignal(std::string("inputButton1"), this,          &GuiInputRouterComponent::OnButton1);
  RegisterSignal(std::string("inputButton1_Release"), this,  &GuiInputRouterComponent::OnButton1_Release);
  RegisterSignal(std::string("Step"), this, &GuiInputRouterComponent::OnStep);

  // really would like to get lambdas to work
  //  void GuiInputRouterComponent::OnPadDown(float frameTime) {
  //  SetNavTypeFromSignal(std::string("inputPadDown"), 1.0f);
  //}
  //static std::function<void(float)> padDown =
  ////static auto padDown = 
  //[this]() -> void {
  //  SetNavTypeFromSignal(std::string("inputPadDown"), 1.0f); };
  //RegisterSignal(std::string("inputPadDown"), this,
  //  padDown);
  //RegisterSignal(std::string("inputPadDown"), this,
  //  [](float frameTime) { SetNavTypeFromSignal(std::string("inputPadDown"), 1.0f); });
}

void GuiInputRouterComponent::OnStep(float delta) {
  m_trackedFrameTime += delta;

  // ugh to support button style, we need to do this every frame??
  // this is terrible
  ImGuiIO& io = ImGui::GetIO();
  //memset(&io.NavInputs[0], 0, sizeof(io.NavInputs));
}

// TODO: All this plumbing can be eliminated if we simply have access to the signal type
// really should make the input structures pass around something more substantial
// then use the same signal for up and down
// and maybe generalize to joystick signal instead of per buttons? round in circles of abstractions
void GuiInputRouterComponent::OnPadUp(float frameTime) {
  SetNavTypeFromSignal(std::string("inputPadUp"), 1.0f);
}

void GuiInputRouterComponent::OnPadUp_Release(float frameTime) {
  SetNavTypeFromSignal(std::string("inputPadUp"), 0.0f);
}

void GuiInputRouterComponent::OnPadDown(float frameTime) {
  SetNavTypeFromSignal(std::string("inputPadDown"), 1.0f);
}

void GuiInputRouterComponent::OnPadDown_Release(float frameTime) {
  SetNavTypeFromSignal(std::string("inputPadDown"), 0.0f);
}

void GuiInputRouterComponent::OnPadLeft(float frameTime) {
  SetNavTypeFromSignal(std::string("inputPadLeft"), 1.0f);
}

void GuiInputRouterComponent::OnPadLeft_Release(float frameTime) {
  SetNavTypeFromSignal(std::string("inputPadLeft"), 0.0f);
}

void GuiInputRouterComponent::OnPadRight(float frameTime) {
  SetNavTypeFromSignal(std::string("inputPadRight"), 1.0f);
}

void GuiInputRouterComponent::OnPadRight_Release(float frameTime) {
  SetNavTypeFromSignal(std::string("inputPadRight"), 0.0f);
}

void GuiInputRouterComponent::OnButton0(float frameTime) {
  SetNavTypeFromSignal(std::string("inputButton0"), 1.0f);
}

void GuiInputRouterComponent::OnButton0_Release(float frameTime) {
  SetNavTypeFromSignal(std::string("inputButton0"), 0.0f);
}

void GuiInputRouterComponent::OnButton1(float frameTime) {
  SetNavTypeFromSignal(std::string("inputButton1"), 1.0f);
}

void GuiInputRouterComponent::OnButton1_Release(float frameTime) {
  SetNavTypeFromSignal(std::string("inputButton1"), 0.0f);
}

} // namespace fd

