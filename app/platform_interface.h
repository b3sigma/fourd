#pragma once

namespace fd {

  // Thus far a belated half-hearted attempt, which might be worse than no
  // attempt at all.

class PlatformWindow;

PlatformWindow* PlatformInit(const char* windowName);
void PlatformShutdown();

} // namespace fd