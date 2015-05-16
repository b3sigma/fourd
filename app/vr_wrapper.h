#pragma once

#include "platform_interface.h"

namespace fd {
  
// Interface class for actual vr.
// Probably silly as ovr is only game in town currently, which means the 2nd
// kind of thing will still be hard.
class VRWrapper {
public:

  static VRWrapper* CreateVR(PlatformWindow* pWindow);

  virtual ~VRWrapper() {}

protected:
  static bool s_Initialized;
  static bool s_UsingVR;

  virtual bool Initialize(PlatformWindow* pWindow) = 0;
};

}; // namespace fd