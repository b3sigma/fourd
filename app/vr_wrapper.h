#pragma once

// Currently only one of these can be defined
// Need to do a cmake rule to make this smooth
// Or see about including both at the same time and having reasonable selection
//#define FD_VR_USE_OCULUS
#define FD_VR_USE_OPENVR

#include <string>
#include "platform_interface.h"
#include "../common/fourmath.h"

namespace fd {

class Camera;
class Texture;
class InputHandler;

// Interface class for actual vr.
// This ended up alright for different builds that work for different setups. That might be fine, but if it turns out a single build for all the headsets is better, the CreateVR function will need to be defined in vr_wrapper.cpp instead and maybe enumerate exposed interfaces from oculus and vive
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

  virtual void HandleInput(float frameTime, InputHandler* inputHandler) {}

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
