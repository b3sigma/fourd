#include "vr_wrapper.h"

#include <memory>

#include <Windows.h>
#include "OVR_CAPI_GL.h"

namespace fd {

bool VRWrapper::s_Initialized = false;
bool VRWrapper::s_UsingVR = false;

// TODO: move this to like ovr_vr_wrapper.cpp or something
class OVRWrapper : public VRWrapper {
public:
  ovrHmd m_HMD; // actually a pointer...

  OVRWrapper() : m_HMD(NULL) {}
  ~OVRWrapper() {
    ovr_Shutdown();
  }
  
  bool Initialize(PlatformWindow* pWindow) {
    if (!ovr_Initialize()) {
      printf("Couldn't init ovr\n");
      return false;
    }

    int ovrIndex = ovrHmd_Detect();
    
    if (ovrIndex == 0 || (NULL == (m_HMD = ovrHmd_Create(0)))) {
      printf("Oculus device not found\n"
          "Trying to create debug oculus device...\n");
      // do we want to #ifdef DEBUG this?
      m_HMD = ovrHmd_CreateDebug(ovrHmd_DK2);
    }

    if (m_HMD == NULL) {
      printf("Oculus device not created:\n");
      return false;
    }
    
    return true;
  }
};

VRWrapper* VRWrapper::CreateVR(PlatformWindow* pWindow) {
  std::unique_ptr<OVRWrapper> vrWrapper(new OVRWrapper);

  if(!vrWrapper->Initialize(pWindow)) {
    return NULL;
  }

  return vrWrapper.release();
}



}; // namespace fd