#include "input_handler.h"

#include <GLFW/glfw3.h>

namespace fd {

void InputHandler::PollJoysticks() {
  for(int j = GLFW_JOYSTICK_1; j <= GLFW_JOYSTICK_LAST; j++) {
    if(glfwJoystickPresent(j)) {
      if((int)m_joysticks.size() < (j+1)) {
        m_joysticks.resize(j+1);
      }

      Joystick& joy = m_joysticks[j];
      joy.m_isPresent = true;
      joy.m_name.assign(glfwGetJoystickName(j));

      int buttonCount = 0;
      const unsigned char* buttons = glfwGetJoystickButtons(j, &buttonCount);
      joy.m_buttons.resize(buttonCount);
      if(buttonCount > 0) {
        memcpy(&joy.m_buttons[0], buttons, buttonCount * sizeof(joy.m_buttons[0]));
      }

      int axesCount = 0;
      const float* axes = glfwGetJoystickAxes(j, &axesCount);
      joy.m_axes.resize(axesCount);
      if(axesCount > 0) {
        memcpy(&joy.m_axes[0], axes, axesCount * sizeof(joy.m_axes[0]));
      }
    }
  }
}


void InputHandler::AddDefaultBindings() {
  JoystickBinding binding;
  binding.m_command.assign("inputForward");
  binding.m_buttonIndex = 1;
  binding.m_isButton = false;
  binding.m_isInverted = false;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputStrafe");
  binding.m_buttonIndex = 0;
  binding.m_isButton = false;
  binding.m_isInverted = false;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputLookUp");
  binding.m_buttonIndex = 3;
  binding.m_isButton = false;
  binding.m_isInverted = false;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputLookRight");
  binding.m_buttonIndex = 4;
  binding.m_isButton = false;
  binding.m_isInverted = true;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputRoll");
  //binding.m_command.assign("inputInside");
  binding.m_buttonIndex = 2; // triggers -1 right, +1 left
  binding.m_isButton = false;
  //binding.m_isInverted = true;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputJump");
  binding.m_buttonIndex = 0;
  binding.m_isButton = true;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputShiftSlice");
  binding.m_buttonIndex = 1;
  binding.m_isButton = true;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputAddQuaxol");
  binding.m_buttonIndex = 3;
  binding.m_isButton = true;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputRemoveQuaxol");
  binding.m_buttonIndex = 2;
  binding.m_isButton = true;
  m_joyBindings.push_back(binding);

  //binding.m_command.assign("inputNextCurrentItem");
  binding.m_command.assign("inputInside");
  binding.m_buttonIndex = 4; // shoulder
  binding.m_isButton = true;
  binding.m_isButtonContinuous = true;
  binding.m_isInverted = false;
  binding.m_spamRepeats = true;
  m_joyBindings.push_back(binding);

  //binding.m_command.assign("inputPrevCurrentItem");
  binding.m_command.assign("inputInside");
  binding.m_buttonIndex = 5; // shoulder button
  binding.m_isButton = true;
  binding.m_isButtonContinuous = true;
  binding.m_isInverted = true;
  binding.m_spamRepeats = true;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputControlsMenu");
  binding.m_buttonIndex = 6; //select
  binding.m_isButton = true;
  m_joyBindings.push_back(binding);

  binding.m_command.assign("inputMainMenu");
  binding.m_buttonIndex = 7; //start
  binding.m_isButton = true;
  m_joyBindings.push_back(binding);

}

void InputHandler::AddInputTarget(ComponentBus* bus) {
  m_inputTargets.push_back(bus);
  //m_inputTarget = bus;
}

void InputHandler::ApplyJoystickInput(float frameTime) {

  for(auto& joy : m_joysticks) {
    if(!joy.m_isPresent) continue;

    for(auto binding : m_joyBindings) {
      if(binding.m_isButton) {
        if(binding.m_buttonIndex >= (int)joy.m_buttons.size())
          continue; // not a possible binding

        if(!joy.m_buttons[binding.m_buttonIndex])
          continue; // only support press for now

        if(!binding.m_spamRepeats) {
          if(binding.m_buttonIndex >= (int)joy.m_lastButtons.size()) {
            continue; // maybe drop events the first frame? oh well
          }

          if(joy.m_lastButtons[binding.m_buttonIndex] ==
              joy.m_buttons[binding.m_buttonIndex]) {
            continue; // same as last state, drop it
          }
        }

        for(auto inputTarget : m_inputTargets) {
          SendAnyInputSignal(inputTarget);
          if(binding.m_isButtonContinuous) {
            float amount = (binding.m_isInverted) ? -1.0f : 1.0f;
            inputTarget->SendSignal(binding.m_command,
                SignalN<float, float>(), frameTime, amount);
          } else {
            inputTarget->SendSignal(binding.m_command,
                SignalN<float>(), frameTime);
          }
        }
      } else { //not a button
        if(binding.m_buttonIndex >= (int)joy.m_axes.size())
          continue; // not a possible binding
        float amount = joy.m_axes[binding.m_buttonIndex];
        if(abs(amount) < binding.m_deadzone)
          continue;
        if(binding.m_isInverted) {
          amount *= -1.0f;
        }

        for(auto inputTarget : m_inputTargets) {
          SendAnyInputSignal(inputTarget);
          inputTarget->SendSignal(binding.m_command,
              SignalN<float, float>(), frameTime, amount);
        }
      }
    }

    joy.m_lastButtons.assign(joy.m_buttons.begin(), joy.m_buttons.end());
  }
}

void InputHandler::SendAnyInputSignal(ComponentBus* target) {
  static const std::string anyInputSignal("AnyInput");
  target->SendSignal(anyInputSignal, SignalN<>());
}


void InputHandler::DoCommand(const std::string& command, float frameTime) {
  for (auto inputTarget : m_inputTargets) {
    inputTarget->SendSignal(command,
      SignalN<float>(), frameTime);

    SendAnyInputSignal(inputTarget);
  }
}

} // namespace fd