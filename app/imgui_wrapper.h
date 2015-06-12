#pragma once

// needed for callback defs
#include <GLFW/glfw3.h>

namespace fd {

class Shader;

class ImGuiWrapper {
public:
  static GLFWwindow* s_glfwWindow;
  static GLFWmousebuttonfun s_chainedMouseButtonCallback;
  static GLFWkeyfun s_chainedKeyCallback;

  static Shader* s_UIRender;
  static Shader* s_UIRenderVR;

public:
  static bool Init(GLFWwindow* glfwWindow,
      GLFWkeyfun keyCallback, GLFWmousebuttonfun mouseButtonCallback);
  static void Shutdown();
  static void NewFrame(float deltaTime);
  static void Render();
  
protected:
  static bool InitOpenGL();

};

}; // namespace fd