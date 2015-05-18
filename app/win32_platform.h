#pragma once
#include <Windows.h>
#include "platform_interface.h"

namespace fd {


class PlatformWindow {
private:
public:
  HWND m_hWnd;
  int m_width;
  int m_height;
  bool m_fullscreen;

  int GetNumDisplays();
  void ToggleFullscreenByMonitorName(const char* name);
  void GetWidthHeight(int* outWidth, int* outHeight);
};

} // namespace fd