#pragma once

namespace fd {

  // Thus far a belated half-hearted attempt, which might be worse than no
  // attempt at all.
  // Weirder, apparently settled on a bizarre mismash of c/c++ style.

class PlatformWindow;

PlatformWindow* PlatformInit(const char* windowName, int width, int height);
void PlatformShutdown();

} // namespace fd