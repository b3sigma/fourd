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

PlatformWindow* PlatformInit(const char* windowName) {
  assert(Win32Platform::s_Platform == NULL);
 
  Win32Platform::s_Platform = new Win32Platform();

  PlatformWindow* pWindow = new PlatformWindow();
  glutSetWindowTitle(windowName);
  pWindow->m_hWnd = FindWindow(NULL, windowName);

  Win32Platform::s_Platform->m_pWindow = pWindow;
  return pWindow;
}

void PlatformShutdown() {
  delete Win32Platform::s_Platform;
  Win32Platform::s_Platform = NULL;
}

} // namespace fd
