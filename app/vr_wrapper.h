#pragma once

#include <string>
#include "platform_interface.h"
#include "../common/fourmath.h"

namespace fd {

class Camera;
class Texture;
//#define FD_VR_USE_OCULUS
#define FD_VR_USE_OPENVR

// Interface class for actual vr.
// Probably silly as ovr is only game in town currently, which means the 2nd
// kind of thing will still be hard.
class VRWrapper {
public:

  static VRWrapper* CreateVR();
  static bool IsUsingVR() { return s_UsingVR; }

  // so with glfw, ovr must happen first, then glfw, then ovr window
  virtual bool InitializeWindow(PlatformWindow* pWindow, float pixelScale) { return false; }

  virtual void StartFrame() {}
  virtual void StartLeftEye(
    Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {}
  virtual void FinishLeftEye(
    Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {}
  virtual void StartRightEye(
    Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {}
  virtual void FinishRightEye(
    Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {}
  virtual void FinishFrame() {}

  virtual void HandleInput() {}

  virtual std::string GetDeviceName() { return std::string(""); }
  virtual bool GetIsDebugDevice() { return true; }

  virtual void SetIsUsingVR(bool usingVR) {}
  virtual void ToggleFullscreen() {}
  virtual void Recenter() {}
  virtual void SetVRPreferredMovementMode(Camera* pCamera) {}

  virtual Texture* GetCurrentRenderColor() { return NULL; }
  virtual Texture* GetCurrentRenderDepth() { return NULL; }

  virtual bool GetPerEyeRenderSize(int& width, int& height) const { return false; }
  virtual bool GetTotalRenderSize(int& width, int& height) const { return false; }

  virtual void SetDebugHeadOrientation(const Mat4f* matrix) {}

  virtual ~VRWrapper() {}

public:
  // nothing like late am deadline coding to add public vars
  bool m_doScreenSaver;
  bool m_hadInput;

  static float s_screenSaverMoveThreshold;
  static float s_screenSaverRotateThreshold;

protected:
  static bool s_Initialized;
  static bool s_UsingVR;


  VRWrapper() {} // prevent direct construction
  virtual bool Initialize() = 0;
};

}; // namespace fd
