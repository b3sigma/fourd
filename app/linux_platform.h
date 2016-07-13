#pragma once

#ifndef WIN32 // pretty sure linux and windows are the only OSs ever

#include "platform_interface.h"
#include <string>

struct GLFWwindow;
struct GLFWmonitor;

namespace fd {

class PlatformWindow {
private:
public:
  // HWND m_hWnd;
  int m_width;
  int m_height;
  bool m_fullscreen;
  bool m_cursorCaptured;
  GLFWwindow* m_glfwWindow;

  int GetNumDisplays();
  void ToggleFullscreenByMonitorName(const char* name);
  static GLFWmonitor* GetRiftMonitorByName(const char* name);
  void ToggleGlfwFullscreenByMonitorName(const char* name);
  void GetWidthHeight(int* outWidth, int* outHeight);
  void CaptureCursor(bool capture);
};

} // namespace fd

#endif //n def WIN32
