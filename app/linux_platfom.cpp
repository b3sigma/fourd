#ifndef WIN32 // sorry future self mac port implementer

#include "platform_interface.h"
#include "linux_platform.h"

#include "unistd.h"

#include <assert.h>
#include <GL/glew.h>

#include <GLFW/glfw3.h>

// #if defined(_MSC_VER) && defined(WIN32)
// #undef APIENTRY
// #define GLFW_EXPOSE_NATIVE_WIN32
// #define GLFW_EXPOSE_NATIVE_WGL
// #include <GLFW/glfw3native.h>
// #endif

namespace fd {

  class LinuxPlatform : public Platform {
  public:
    static LinuxPlatform* s_Platform;

    PlatformWindow* m_pWindow;

    LinuxPlatform() : m_pWindow(NULL) {}
    ~LinuxPlatform() {
      delete m_pWindow;
    }
  };
  LinuxPlatform* LinuxPlatform::s_Platform = NULL;

PlatformWindow* Platform::Init(
    const char* windowName, int width, int height) {
  assert(LinuxPlatform::s_Platform == NULL);

  LinuxPlatform::s_Platform = new LinuxPlatform();

  PlatformWindow* pWindow = new PlatformWindow();
  // pWindow->m_hWnd = FindWindow(NULL, windowName);
  pWindow->m_width = width;
  pWindow->m_height = height;
  pWindow->m_fullscreen = false;
  pWindow->m_cursorCaptured = false;

  LinuxPlatform::s_Platform->m_pWindow = pWindow;
  return pWindow;
}

void Platform::Shutdown() {
  delete LinuxPlatform::s_Platform;
  LinuxPlatform::s_Platform = NULL;
}

bool Platform::GetNextFileName(const char* fileMatch,
    const char* currentFile, std::string& foundFile) {
  return false;
  // TODO: do
  // if(!fileMatch) return false;
  //
  // WIN32_FIND_DATA findData;
  // HANDLE hFind = FindFirstFile(fileMatch, &findData);
  // if(hFind == INVALID_HANDLE_VALUE) {
  //   printf("GetNextFileName failed:%d for %s", GetLastError(), fileMatch);
  //   return false;
  // }
  //
  // foundFile.assign(&findData.cFileName[0]);
  //
  // if(!currentFile) {
  //   return true; //first match is fine
  // }
  //
  // bool nextOne = false;
  // while(true) {
  //   if (strstr(&findData.cFileName[0], currentFile)) {
  //     nextOne = true;
  //   }
  //   if(0 == FindNextFile(hFind, &findData)) {
  //     return true; //hit the end of the list, return what was the first one
  //   }
  //
  //   if(nextOne) {
  //     foundFile.assign(&findData.cFileName[0]);
  //     return true;
  //   }
  // }
  //
  // return true;
}

void PlatformWindow::GetWidthHeight(int* outWidth, int* outHeight) {
  *outWidth = m_width;
  *outHeight = m_height;
}

// class MonitorsInfo {
// public:
//   enum { MaxNumMonitors = 8 };
//   HMONITOR m_hMonitors[MaxNumMonitors];
//   int m_numMonitors;
//   MonitorsInfo() : m_numMonitors(0) {}
// };
//
// BOOL CALLBACK MonitorEnumProcCallback(
//     HMONITOR hMon, HDC hDC, LPRECT pRect, LPARAM pMonitorsInfo) {
//   MonitorsInfo* monitorsInfo = (MonitorsInfo*)pMonitorsInfo;
//   if(monitorsInfo->m_numMonitors >= MonitorsInfo::MaxNumMonitors) {
//     return FALSE;
//   }
//
//   monitorsInfo->m_hMonitors[monitorsInfo->m_numMonitors] = hMon;
//   monitorsInfo->m_numMonitors++;
//   return TRUE;
// }

// Follows logic out of oculus sdk for regarding mirrored displays as single.
int PlatformWindow::GetNumDisplays() {
  return 1;
  // MonitorsInfo monitorsInfo;
  // EnumDisplayMonitors(NULL /*hdc*/, NULL /*clipRect*/,
  //     MonitorEnumProcCallback, (LPARAM)&monitorsInfo);
  //
  // int numPrimary = 0;
  // MONITORINFOEX monitorInfo;
  // monitorInfo.cbSize = sizeof(monitorInfo);
  // for(int m = 0; m < monitorsInfo.m_numMonitors; ++m) {
  //   GetMonitorInfo(monitorsInfo.m_hMonitors[m], &monitorInfo);
  //   if(monitorInfo.dwFlags & MONITORINFOF_PRIMARY) {
  //     numPrimary++;
  //   }
  // }
  //
  // if(numPrimary > 1) {
  //   return 1; // so mirrored displays all count as primary, thus just return 1
  // } else {
  //   return monitorsInfo.m_numMonitors;
  // }
}

void PlatformWindow::ToggleFullscreenByMonitorName(const char* name) {
  // m_fullscreen = !m_fullscreen;
  //
  // MonitorsInfo monitors;
  // EnumDisplayMonitors(NULL /*hdc*/, NULL /*clipRect*/,
  //     MonitorEnumProcCallback, (LPARAM)&monitors);
  //
  // RECT bestMonitorRect;
  // bool foundPrimary = false;
  // RECT otherMonitorRect;
  // bool foundOther = false;
  //
  // MONITORINFOEX monitorInfo;
  // monitorInfo.cbSize = sizeof(monitorInfo);
  // for(int m = 0; m < monitors.m_numMonitors; ++m) {
  //   if(!GetMonitorInfo(monitors.m_hMonitors[m], &monitorInfo))
  //     continue;
  //   if(strstr(name, monitorInfo.szDevice)
  //       || strstr(monitorInfo.szDevice, name)) {
  //     bestMonitorRect = monitorInfo.rcMonitor;
  //     foundPrimary = true; // this is the target, always set
  //   } else if(monitorInfo.dwFlags & MONITORINFOF_PRIMARY) {
  //     if(!foundPrimary) {
  //       bestMonitorRect = monitorInfo.rcMonitor;
  //       foundPrimary = true; // a primary is backup
  //     }
  //     otherMonitorRect = monitorInfo.rcMonitor;
  //     foundOther = true; // other is useful
  //   }
  // }
  //
  // // drop cursor capture for the moment
  // bool wasCursorCaptured = m_cursorCaptured;
  // if(wasCursorCaptured) {
  //   CaptureCursor(false);
  // }
  //
  // if(m_fullscreen) {
  //   //if(foundPrimary) {
  //   //  int width = bestMonitorRect.right - bestMonitorRect.left;
  //   //  int height = bestMonitorRect.bottom - bestMonitorRect.top;
  //   //  SetWindowPos(m_hWnd, HWND_TOPMOST,
  //   //      bestMonitorRect.left, bestMonitorRect.top, width, height,
  //   //      SWP_SHOWWINDOW | SWP_NOZORDER | SWP_FRAMECHANGED);
  //   //}
  //   // Mixing of win32 and glut code is... interesting
  //   //glutFullScreen();
  //
  //   GLFWmonitor* monitor = GetRiftMonitorByName(name);
  //   if(!monitor) {
  //     monitor = glfwGetPrimaryMonitor();
  //   }
  //   if(monitor) {
  //     const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
  //     glfwSetWindowMonitor(m_glfwWindow, monitor,
  //         vidMode->width, vidMode->height, GLFW_DONT_CARE);
  //   }
  // } else {
  //   glfwSetWindowMonitor(m_glfwWindow, NULL /*monitor*/,
  //       m_width, m_height, GLFW_DONT_CARE);
  //
  //   //glutLeaveFullScreen();
  //   //if(foundOther) {
  //   //  SetWindowPos(m_hWnd, HWND_TOPMOST,
  //   //      otherMonitorRect.left, otherMonitorRect.top, m_width, m_height,
  //   //      SWP_SHOWWINDOW | SWP_NOZORDER | SWP_FRAMECHANGED);
  //   //}
  // }
  //
  // // recapture with new size/pos
  // if(wasCursorCaptured) {
  //   CaptureCursor(true);
  // }

}

GLFWmonitor* PlatformWindow::GetRiftMonitorByName(const char* name) {
  // int monitorCount;
  // GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
  // for(int m = 0; m < monitorCount; m++) {
  //   const char* monitorName = glfwGetWin32Monitor(monitors[m]);
  //   if(strstr(name, monitorName) == 0) {
  //     return monitors[m];
  //   }
  // }
  return NULL;
}

void PlatformWindow::CaptureCursor(bool capture) {
  // m_cursorCaptured = capture;
  // if (capture) {
  //   RECT windowRect; // will include border
  //   GetWindowRect(m_hWnd, &windowRect);
  //   RECT clientRect; // local window space
  //   GetClientRect(m_hWnd, &clientRect);
  //
  //   RECT interiorRect = clientRect;
  //   interiorRect.left += windowRect.left;
  //   interiorRect.right += windowRect.left;
  //   interiorRect.top += windowRect.top;
  //   interiorRect.bottom += windowRect.top;
  //
  //   // This is actually wrong, we aren't handling the border correctly,
  //   // but it doesn't matter as the mouse move wrap handles it.
  //   BOOL result = ClipCursor(&interiorRect);
  //
  //   glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // } else {
  //   ClipCursor(NULL);
  //   glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  // }
}

void Platform::ThreadSleep(unsigned long milliseconds) {
  assert(false); // just haven't tried it yet
  //usleep(milliseconds);
}

} // namespace fd

#endif //n WIN32
