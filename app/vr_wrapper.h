#pragma once

#include "platform_interface.h"

namespace fd {
  
// Interface class for actual vr.
// Probably silly as ovr is only game in town currently, which means the 2nd
// kind of thing will still be hard.
class VRWrapper {
public:

  static VRWrapper* CreateVR(PlatformWindow* pWindow);
  static bool IsUsingVR() { return s_UsingVR; }
  
  virtual void StartFrame() {}
  virtual void StartLeftEye() {}
  virtual void StartRightEye() {}
  virtual void FinishFrame() {}

  virtual void SetIsUsingVR(bool usingVR) {}
  virtual void ToggleFullscreen() {}

  virtual ~VRWrapper() {}

protected:
  static bool s_Initialized;
  static bool s_UsingVR;

  VRWrapper() {} // prevent direct construction
  virtual bool Initialize(PlatformWindow* pWindow) = 0;
};

}; // namespace fd