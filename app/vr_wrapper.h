#pragma once

#include "platform_interface.h"
#include "..\common\fourmath.h"

namespace fd {

class Camera;
  
// Interface class for actual vr.
// Probably silly as ovr is only game in town currently, which means the 2nd
// kind of thing will still be hard.
class VRWrapper {
public:

  static VRWrapper* CreateVR();
  static bool IsUsingVR() { return s_UsingVR; }

  // so with glfw, ovr must happen first, then glfw, then ovr window
  virtual bool InitializeWindow(PlatformWindow* pWindow) { return false; }
  
  virtual void StartFrame() {}
  virtual void StartLeftEye(Camera* pCamera) {}
  virtual void StartRightEye(Camera* pCamera) {}
  virtual void FinishFrame() {}

  virtual void SetIsUsingVR(bool usingVR) {}
  virtual void ToggleFullscreen() {}
  virtual void Recenter() {}

  virtual int GetRenderWidth() const { return 0; }
  virtual int GetRenderHeight() const { return 0; }

  virtual void SetDebugHeadOrientation(const Mat4f* matrix) {}

  virtual ~VRWrapper() {}

protected:
  static bool s_Initialized;
  static bool s_UsingVR;

  VRWrapper() {} // prevent direct construction
  virtual bool Initialize() = 0;
};

}; // namespace fd