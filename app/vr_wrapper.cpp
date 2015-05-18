#include "vr_wrapper.h"
#include "texture.h"

#include <memory>

#include <Windows.h>
#include "OVR_CAPI_GL.h"

#include "glhelper.h"

namespace fd {

bool VRWrapper::s_Initialized = false;
bool VRWrapper::s_UsingVR = false;

// TODO: move this to like ovr_vr_wrapper.cpp or something
class OVRWrapper : public VRWrapper {
public:
  ovrHmd m_HMD; // actually a pointer...

  Texture* m_eyeRenderTex[2];
  Texture* m_eyeDepthTex[2];

  OVRWrapper() : m_HMD(NULL) {
    for(int e = 0; e < 2; e++) {
      m_eyeRenderTex[e] = NULL;
      m_eyeDepthTex[e] = NULL;
    }
  }
  ~OVRWrapper() {
    for(int e = 0; e < 2; e++) {
      delete m_eyeRenderTex[e];
      delete m_eyeDepthTex[e];
    }
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
    WasGLErrorPlusPrint();

    bool createSuccess = true;
    for (int e = 0; e < 2; e++) {
      ovrSizei recommendedFovTexSize = ovrHmd_GetFovTextureSize(
          m_HMD, (ovrEyeType)e, m_HMD->DefaultEyeFov[e],
          1.0f /*pixelsPerDisplayPixel*/);  
      WasGLErrorPlusPrint();
      m_eyeRenderTex[e] = new Texture();
      createSuccess &= m_eyeRenderTex[e]->CreateRenderTarget(
          recommendedFovTexSize.w, recommendedFovTexSize.h); 
      m_eyeDepthTex[e] = new Texture();
      createSuccess &= m_eyeDepthTex[e]->CreateDepthTarget(
          recommendedFovTexSize.w, recommendedFovTexSize.h); 
    }
    if(!createSuccess) {
      printf("VR Render/depth target creation failed\n");
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