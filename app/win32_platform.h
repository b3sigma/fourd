#pragma once
#include <Windows.h>
#include "platform_interface.h"
#include <string>

struct GLFWwindow;

namespace fd {

class PlatformWindow {
private:
public:
  HWND m_hWnd;
  int m_width;
  int m_height;
  bool m_fullscreen;
  bool m_cursorCaptured;
  GLFWwindow* m_glfwWindow;
  
  int GetNumDisplays();
  void ToggleFullscreenByMonitorName(const char* name);
  void GetWidthHeight(int* outWidth, int* outHeight);
  void CaptureCursor(bool capture);
};

} // namespace fd