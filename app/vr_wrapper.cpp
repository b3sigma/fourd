#include "vr_wrapper.h"

namespace fd {

bool VRWrapper::s_Initialized = false;
bool VRWrapper::s_UsingVR = false;

float VRWrapper::s_screenSaverMoveThreshold = 0.00003f;
float VRWrapper::s_screenSaverRotateThreshold = 0.0001f;

#if !defined(FD_VR_USE_OCULUS) && !defined(FD_VR_USE_OPENVR)

VRWrapper* VRWrapper::CreateVR() {
  return NULL;
}
#endif // !defined(FD_VR_USE_OCULUS) && !defined(FD_VR_USE_OPENVR)

} // namespace fd
