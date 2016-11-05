#pragma once
#include <string>

namespace fd {

  // Thus far a belated half-hearted attempt, which might be worse than no
  // attempt at all.
  // Weirder, apparently settled on a bizarre mismash of c/c++ style.

class PlatformWindow;

class Platform {
public:

  static PlatformWindow* Init(const char* windowName, int width, int height);
  static void Shutdown();

  // nextFile will be fileMatch relative
  static bool GetNextFileName(const char* fileMatch,
      const char* currentFile, std::string& nextFile);

  static void ThreadSleep(unsigned long milliseconds);
};



} // namespace fd