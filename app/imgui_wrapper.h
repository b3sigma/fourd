#pragma once

// needed for callback defs
#include <GLFW/glfw3.h>
#include "..\common\fourmath.h"

namespace fd {

class Shader;
class Render;

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
  static void NewFrame(float deltaTime, int renderWidth, int renderHeight);
  static void Render(float frameTime, const Vec2f& offset, Render* renderer, bool doUpdate);
  
protected:
  static bool InitOpenGL();

};

}; // namespace fd