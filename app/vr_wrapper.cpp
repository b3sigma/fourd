#include "vr_wrapper.h"
#include "texture.h"

#include <memory>

#include <Windows.h>
// uh, weird this is manual
#define OVR_OS_WIN32
#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"

#include <GL/freeglut.h>

#include "glhelper.h"
#include "win32_platform.h"
#include "../common/fourmath.h"
#include "../common/camera.h"

namespace fd {

bool VRWrapper::s_Initialized = false;
bool VRWrapper::s_UsingVR = false;

// TODO: move this to like ovr_vr_wrapper.cpp or something
class OVRWrapper : public VRWrapper {
public:
  PlatformWindow* m_pWindow;
  ovrHmd m_HMD; // actually a pointer...

  Texture* m_eyeRenderTex[2];
  Texture* m_eyeDepthTex[2];

  ovrEyeRenderDesc m_eyeDesc[2];
  ovrPosef m_eyeRenderPose[2];

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

    m_pWindow = pWindow;
    
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

    const float pixelsPerDisplayPixel = 0.25f; // 1.0f;
    bool createSuccess = true;
    for (int e = 0; e < 2; e++) {
      ovrSizei recommendedFovTexSize = ovrHmd_GetFovTextureSize(
          m_HMD, (ovrEyeType)e, m_HMD->DefaultEyeFov[e],
          pixelsPerDisplayPixel);
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

    ovrGLConfig config;
    config.OGL.Header.API = ovrRenderAPI_OpenGL;
    config.OGL.Header.BackBufferSize = m_HMD->Resolution;
    config.OGL.Header.Multisample = 0;
    config.OGL.Window = pWindow->m_hWnd;

    int ovrDistortionCaps = ovrDistortionCap_Vignette
        | ovrDistortionCap_TimeWarp
        | ovrDistortionCap_Overdrive;
    ovrHmd_ConfigureRendering(m_HMD, &config.Config, ovrDistortionCaps,
        m_HMD->DefaultEyeFov, m_eyeDesc);

    int ovrCaps = ovrHmdCap_DynamicPrediction | ovrHmdCap_LowPersistence;
    ovrHmd_SetEnabledCaps(m_HMD, ovrCaps);

    ovrHmd_AttachToWindow(m_HMD, pWindow->m_hWnd,
        NULL /*destRect*/, NULL /*srcRect*/);

    int trackingCaps = ovrTrackingCap_Orientation
        | ovrTrackingCap_MagYawCorrection
        | ovrTrackingCap_Position;
    ovrHmd_ConfigureTracking(m_HMD, trackingCaps, 0);

    ovrHmd_DismissHSWDisplay(m_HMD);

    s_Initialized = true;
    //s_UsingVR = true;

    return true;
  }

  virtual void StartFrame() {
    ovrHmd_BeginFrame(m_HMD, 0);

    ovrVector3f viewOffsets[2] = {
        m_eyeDesc[0].HmdToEyeViewOffset,
        m_eyeDesc[1].HmdToEyeViewOffset };

    ovrHmd_GetEyePoses(m_HMD, 0, viewOffsets, m_eyeRenderPose,
        NULL /*trackingState*/);
  }

  void StartEye(int eye) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_eyeRenderTex[eye]->m_framebuffer_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, m_eyeRenderTex[eye]->m_texture_id, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, m_eyeDepthTex[eye]->m_texture_id, 0);

    glViewport(0, 0,
        m_eyeRenderTex[eye]->m_width, m_eyeRenderTex[eye]->m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void UpdateCameraRenderMatrix(int eye, Camera* pCamera) {

    const ovrVector3f& localPosOvr = m_eyeRenderPose[eye].Position;

    const float worldScale = 20.0f;
    Vec4f localOffset(localPosOvr.x, localPosOvr.z, localPosOvr.y, 0.0f);
    localOffset *= worldScale;

    Vec4f offset = pCamera->_cameraMatrix.transpose().transform(localOffset);
    pCamera->_renderPos = pCamera->_cameraPos + offset;
    
    const ovrQuatf& localQuatOvr = m_eyeRenderPose[eye].Orientation;
    Quatf localEyeQuat(localQuatOvr.w, localQuatOvr.x, localQuatOvr.y, localQuatOvr.z);
    Mat4f localEye;
    localEye.storeQuat3dRotation(localEyeQuat);

    pCamera->_renderMatrix = localEye * pCamera->_cameraMatrix;
    //pCamera->_renderMatrix = pCamera->_cameraMatrix;
  }

  virtual void StartLeftEye(Camera* pCamera) {
    const int eye = 0;
    StartEye(eye);
    UpdateCameraRenderMatrix(eye, pCamera);
  }

  virtual void StartRightEye(Camera* pCamera) {
    const int eye = 1;
    StartEye(eye);
    UpdateCameraRenderMatrix(eye, pCamera);
  }

  virtual void FinishFrame() {
    ovrGLTexture eyeTex[2];
    for(int e = 0; e < 2; e++) {
      eyeTex[e].OGL.Header.API = ovrRenderAPI_OpenGL;
      eyeTex[e].OGL.Header.TextureSize.w = m_eyeRenderTex[e]->m_width;
      eyeTex[e].OGL.Header.TextureSize.h = m_eyeRenderTex[e]->m_height;
      eyeTex[e].OGL.Header.RenderViewport.Pos.x = 0;
      eyeTex[e].OGL.Header.RenderViewport.Pos.y = 0;
      eyeTex[e].OGL.Header.RenderViewport.Size = eyeTex[e].OGL.Header.TextureSize;
      eyeTex[e].OGL.TexId = m_eyeRenderTex[e]->m_texture_id;
    }
    //ovrTexture eyeTexStruct[2];
    //eyeTexStruct[0] = eyeTex[0].Texture;
    // uh this seems weird.... but parallel to sample so far
    ovrHmd_EndFrame(m_HMD, m_eyeRenderPose, &eyeTex[0].Texture);
  }

  virtual void SetIsUsingVR(bool usingVR) {
    if(!s_Initialized) return;
    if(s_UsingVR == usingVR) return;

    if(s_UsingVR) {
      s_UsingVR = false;
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      int restoreWidth;
      int restoreHeight;
      m_pWindow->GetWidthHeight(&restoreWidth, &restoreHeight);
      glViewport(0, 0, restoreWidth, restoreHeight);

    } else {
      s_UsingVR = true;
    }
  }

  virtual void ToggleFullscreen() {
    if(!m_HMD) return;
    //if(!(m_HMD->HmdCaps & ovrHmdCap_ExtendDesktop)) return;

    m_pWindow->ToggleFullscreenByMonitorName(m_HMD->DisplayDeviceName);
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