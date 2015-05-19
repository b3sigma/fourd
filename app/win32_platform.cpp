#include "platform_interface.h"
#include "win32_platform.h"

#include <assert.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

namespace fd {

  class Win32Platform {
  public:
    static Win32Platform* s_Platform; // your mom hates singletons

    PlatformWindow* m_pWindow;

    Win32Platform() : m_pWindow(NULL) {}
    ~Win32Platform() {
      delete m_pWindow;
    }
  };
  Win32Platform* Win32Platform::s_Platform = NULL;

PlatformWindow* PlatformInit(const char* windowName, int width, int height) {
  assert(Win32Platform::s_Platform == NULL);
 
  Win32Platform::s_Platform = new Win32Platform();

  PlatformWindow* pWindow = new PlatformWindow();
  glutSetWindowTitle(windowName);
  pWindow->m_hWnd = FindWindow(NULL, windowName);
  pWindow->m_width = width;
  pWindow->m_height = height;
  pWindow->m_fullscreen = false;

  Win32Platform::s_Platform->m_pWindow = pWindow;
  return pWindow;
}

void PlatformShutdown() {
  delete Win32Platform::s_Platform;
  Win32Platform::s_Platform = NULL;
}

void PlatformWindow::GetWidthHeight(int* outWidth, int* outHeight) {
  *outWidth = m_width;
  *outHeight = m_height;
}

class MonitorsInfo {
public:
  enum { MaxNumMonitors = 8 };
  HMONITOR m_hMonitors[MaxNumMonitors];
  int m_numMonitors;
  MonitorsInfo() : m_numMonitors(0) {}
};

BOOL CALLBACK MonitorEnumProcCallback(
    HMONITOR hMon, HDC hDC, LPRECT pRect, LPARAM pMonitorsInfo) {
  MonitorsInfo* monitorsInfo = (MonitorsInfo*)pMonitorsInfo;
  if(monitorsInfo->m_numMonitors >= MonitorsInfo::MaxNumMonitors) {
    return FALSE;
  }

  monitorsInfo->m_hMonitors[monitorsInfo->m_numMonitors] = hMon;
  monitorsInfo->m_numMonitors++;
  return TRUE;
}

// Follows logic out of oculus sdk for regarding mirrored displays as single.
int PlatformWindow::GetNumDisplays() {
  MonitorsInfo monitorsInfo;
  EnumDisplayMonitors(NULL /*hdc*/, NULL /*clipRect*/,
      MonitorEnumProcCallback, (LPARAM)&monitorsInfo); 

  int numPrimary = 0;
  MONITORINFOEX monitorInfo;
  monitorInfo.cbSize = sizeof(monitorInfo);
  for(int m = 0; m < monitorsInfo.m_numMonitors; ++m) {
    GetMonitorInfo(monitorsInfo.m_hMonitors[m], &monitorInfo);
    if(monitorInfo.dwFlags & MONITORINFOF_PRIMARY) {
      numPrimary++;
    }
  }

  if(numPrimary > 1) {
    return 1; // so mirrored displays all count as primary, thus just return 1
  } else {
    return monitorsInfo.m_numMonitors;
  }
}

void PlatformWindow::ToggleFullscreenByMonitorName(const char* name) {
  m_fullscreen = !m_fullscreen;

  MonitorsInfo monitors;
  EnumDisplayMonitors(NULL /*hdc*/, NULL /*clipRect*/,
      MonitorEnumProcCallback, (LPARAM)&monitors);

  RECT bestMonitorRect;
  bool foundPrimary = false;  
  RECT otherMonitorRect;
  bool foundOther = false;

  MONITORINFOEX monitorInfo;
  monitorInfo.cbSize = sizeof(monitorInfo);
  for(int m = 0; m < monitors.m_numMonitors; ++m) {
    if(!GetMonitorInfo(monitors.m_hMonitors[m], &monitorInfo))
      continue;
    if(strstr(name, monitorInfo.szDevice) 
        || strstr(monitorInfo.szDevice, name)) {
      bestMonitorRect = monitorInfo.rcMonitor;
      foundPrimary = true; // this is the target, always set
    } else if(monitorInfo.dwFlags & MONITORINFOF_PRIMARY) {
      if(!foundPrimary) {
        bestMonitorRect = monitorInfo.rcMonitor;
        foundPrimary = true; // a primary is backup
      }
      otherMonitorRect = monitorInfo.rcMonitor;
      foundOther = true; // other is useful
    }
  }

  if(m_fullscreen) {
    if(foundPrimary) {
      int width = bestMonitorRect.right - bestMonitorRect.left;
      int height = bestMonitorRect.bottom - bestMonitorRect.top;
      SetWindowPos(m_hWnd, HWND_TOPMOST,
          bestMonitorRect.left, bestMonitorRect.top, width, height,
          SWP_SHOWWINDOW | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    // Mixing of win32 and glut code is... interesting
    glutFullScreen();
  } else {
    glutLeaveFullScreen();
    if(foundOther) {
      SetWindowPos(m_hWnd, HWND_TOPMOST,
          otherMonitorRect.left, otherMonitorRect.top, m_width, m_height,
          SWP_SHOWWINDOW | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
  } 
}

void PlatformWindow::CaptureCursor(bool capture) {
  if (capture) {
    RECT windowRect; // will include border
    GetWindowRect(m_hWnd, &windowRect);
    RECT clientRect; // local window space
    GetClientRect(m_hWnd, &clientRect);

    RECT interiorRect = clientRect;
    interiorRect.left += windowRect.left;
    interiorRect.right += windowRect.left;
    interiorRect.top += windowRect.top;
    interiorRect.bottom += windowRect.top;

    // This is actually wrong, we aren't handling the border correctly,
    // but it doesn't matter as the mouse move wrap handles it.
    BOOL result = ClipCursor(&interiorRect);

    glutSetCursor(GLUT_CURSOR_NONE);
  } else {
    ClipCursor(NULL);
    glutSetCursor(GLUT_CURSOR_INHERIT);
  }
}

} // namespace fd
