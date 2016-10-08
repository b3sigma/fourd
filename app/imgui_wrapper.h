#pragma once

// needed for callback defs
#include <GLFW/glfw3.h>
#include "../common/fourmath.h"
#include "imgui_console.h"

namespace fd {

class Shader;
class Render;

class ImGuiWrapper {
public: // needs to be public
  static bool s_bGuiDisabled;
public: // probably just because of lazy
  static GLFWwindow* s_glfwWindow;
  static GLFWmousebuttonfun s_chainedMouseButtonCallback;
  static GLFWkeyfun s_chainedKeyCallback;

  static Shader* s_UIRender;
  static Shader* s_UIRenderVR;

public:
  static bool Init(GLFWwindow* glfwWindow,
      GLFWkeyfun keyCallback, GLFWmousebuttonfun mouseButtonCallback,
      ConsoleInterface::OnCommandCallback);
  static void Shutdown();
  static void NewFrame(float deltaTime, int renderWidth, int renderHeight);
  static void Render(float frameTime, const Vec2f& offset, Render* renderer, bool doUpdate);

  static void ToggleControllerMenu();
  static bool ToggleResetMenu(); // returns true if window is now shown

protected:
  static bool InitOpenGL();

};

}; // namespace fd
