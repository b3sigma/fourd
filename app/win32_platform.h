#pragma once
#include <Windows.h>
#include "platform_interface.h"

namespace fd {


class PlatformWindow {
private:
public:
  HWND  m_hWnd;
};

} // namespace fd