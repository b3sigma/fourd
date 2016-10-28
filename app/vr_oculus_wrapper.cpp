#include "vr_wrapper.h"
#ifdef FD_VR_USE_OCULUS

#include "texture.h"

#include <memory>

#ifdef WIN32
#include <Windows.h>
// looks like oculus is windows only at this point anyway...
#define OVR_OS_WIN32
#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"
#endif

#include "glhelper.h"
#include "render.h"
#include "win32_platform.h"
#include "../common/fourmath.h"
#include "../common/camera.h"

namespace fd {

#if defined(WIN32)
class OVRWrapper : public VRWrapper {
public:
  PlatformWindow* m_pWindow;
  ovrHmd m_HMD; // actually a pointer...
  bool m_isDebugDevice;

  Texture* m_eyeRenderTex[2];
  Texture* m_eyeDepthTex[2];

  ovrEyeRenderDesc m_eyeDesc[2];
  ovrPosef m_eyeRenderPose[2];
  ovrPosef m_lastEyeRenderPose[2];

  int m_eyeRenderWidth;
  int m_eyeRenderHeight;

  const Mat4f* m_debugHeadPose;

  OVRWrapper() : m_HMD(NULL), m_debugHeadPose(NULL)
      , m_eyeRenderWidth(0), m_eyeRenderHeight(0)
      , m_isDebugDevice(true)
  {
    for(int e = 0; e < 2; e++) {
      m_eyeRenderTex[e] = NULL;
      m_eyeDepthTex[e] = NULL;
    }
    m_doScreenSaver = false;
    m_hadInput = false;
  }

  ~OVRWrapper() {
    for(int e = 0; e < 2; e++) {
      delete m_eyeRenderTex[e];
      delete m_eyeDepthTex[e];
    }
    ovr_Shutdown();
  }

  bool Initialize() {
    if (!ovr_Initialize()) {
      printf("Couldn't init ovr\n");
      return false;
    }

    int ovrIndex = ovrHmd_Detect();

    m_isDebugDevice = false;
    if (ovrIndex == 0 || (NULL == (m_HMD = ovrHmd_Create(0)))) {
      printf("Oculus device not found\n"
          "Trying to create debug oculus device...\n");
      // do we want to #ifdef DEBUG this?
      m_HMD = ovrHmd_CreateDebug(ovrHmd_DK2);
      m_isDebugDevice = true;
    }

    if (m_HMD == NULL) {
      printf("Oculus device not created:\n");
      return false;
    }

    int ovrCaps = ovrHmdCap_DynamicPrediction | ovrHmdCap_LowPersistence;
    ovrHmd_SetEnabledCaps(m_HMD, ovrCaps);

    int trackingCaps = ovrTrackingCap_Orientation
        | ovrTrackingCap_MagYawCorrection
        | ovrTrackingCap_Position;
    ovrHmd_ConfigureTracking(m_HMD, trackingCaps, 0);

    return true;
  }

  virtual bool InitializeWindow(PlatformWindow* pWindow, float pixelScale) {
    m_pWindow = pWindow;

    const float pixelsPerDisplayPixel = pixelScale; //0.25f; // 1.0f;
    bool createSuccess = true;
    for (int e = 0; e < 2; e++) {
      ovrSizei recommendedFovTexSize = ovrHmd_GetFovTextureSize(
          m_HMD, (ovrEyeType)e, m_HMD->DefaultEyeFov[e],
          pixelsPerDisplayPixel);
      m_eyeRenderWidth = recommendedFovTexSize.w;
      m_eyeRenderHeight = recommendedFovTexSize.h;
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
    config.OGL.Window = m_pWindow->m_hWnd;

    int ovrDistortionCaps = ovrDistortionCap_Vignette
        | ovrDistortionCap_Chromatic
        | ovrDistortionCap_TimeWarp
        | ovrDistortionCap_Overdrive;
    ovrHmd_ConfigureRendering(m_HMD, &config.Config, ovrDistortionCaps,
        m_HMD->DefaultEyeFov, m_eyeDesc);

    ovrHmd_AttachToWindow(m_HMD, m_pWindow->m_hWnd,
        NULL /*destRect*/, NULL /*srcRect*/);

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

    if(m_doScreenSaver) {
      // reset screensaver detection state
      m_lastEyeRenderPose[0] = m_eyeRenderPose[0];
      m_lastEyeRenderPose[1] = m_eyeRenderPose[1];
      m_hadInput = false;
    }
    ovrHmd_GetEyePoses(m_HMD, 0, viewOffsets, m_eyeRenderPose,
        NULL /*trackingState*/);
  }

  void StartEye(int eye, Texture** outRenderColor, Texture** outRenderDepth) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_eyeRenderTex[eye]->m_framebuffer_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, m_eyeRenderTex[eye]->m_texture_id, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, m_eyeDepthTex[eye]->m_texture_id, 0);

    if(outRenderColor) {
      *outRenderColor = m_eyeRenderTex[eye];
    }
    if(outRenderDepth) {
      *outRenderDepth = m_eyeDepthTex[eye];
    }

    glViewport(0, 0,
        m_eyeRenderTex[eye]->m_width, m_eyeRenderTex[eye]->m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }


  virtual void SetDebugHeadOrientation(const Mat4f* matrix) {
    m_debugHeadPose = matrix;
  }

  void UpdateCameraRenderMatrix(int eye, Camera* pCamera) {

    const ovrQuatf& localQuatOvr = m_eyeRenderPose[eye].Orientation;
    Quatf localEyeQuat(localQuatOvr.w, localQuatOvr.x, localQuatOvr.y, localQuatOvr.z);
    Mat4f localEye;
    localEye.storeQuat3dRotation(localEyeQuat);

    if(m_debugHeadPose) {
      localEye = *m_debugHeadPose;
    }

    const float worldScale = 20.0f;
    static bool simpleEyeOffset = true;
    if(simpleEyeOffset) {
      const ovrVector3f& localPosOvr = m_eyeDesc[eye].HmdToEyeViewOffset;
      Vec4f ovrEyePos(-localPosOvr.x, localPosOvr.y, localPosOvr.z, 0.0f);
      ovrEyePos *= worldScale;
      ovrEyePos = pCamera->_renderMatrix.transpose().transform(ovrEyePos);
      //pCamera->_renderPos = pCamera->_cameraPos + ovrEyePos;
      pCamera->UpdateRenderMatrix(&localEye, &ovrEyePos);

    } else {
      const ovrVector3f& ovrEyeZero = m_eyeRenderPose[0].Position;
      const ovrVector3f& ovrEyeOne = m_eyeRenderPose[1].Position;

      Vec4f ovrEyeDifference(
          ovrEyeOne.x - ovrEyeZero.x,
          ovrEyeOne.y - ovrEyeZero.y,
          ovrEyeOne.z - ovrEyeZero.z,
          0.0f);

      const ovrVector3f& localPosOvr = m_eyeRenderPose[eye].Position;
      static int transposer[3] = {0, 1, 2};
      //static Vec4f sign(1.0f, 1.0f, 1.0f, 1.0f);
      static Vec4f sign(-1.0f, 1.0f, 1.0f, 1.0f);
      Vec4f ovrEyePos(localPosOvr.x, localPosOvr.y, localPosOvr.z, 0.0f);

      // TODO: try to convert ovr into local using simple sign switch
      // then independently do eye and forehead transformations
      Vec4f ovrForehead;
      if(eye == 0) {
        ovrForehead = ovrEyePos - (ovrEyeDifference * 0.5f);
      } else {
        ovrForehead = ovrEyePos - (ovrEyeDifference * -0.5f);
      }

      ovrEyePos *= worldScale;
      Vec4f localOffset(
          ovrEyePos[transposer[0]] * sign.x,
          ovrEyePos[transposer[1]] * sign.y,
          ovrEyePos[transposer[2]] * sign.z,
          0.0f);

      static float amount = (float)PI / 2.0f;
      static int targetAxis = 2;
      static int sourceAxis = 1;
      Mat4f matArbitrary(Mat4f().storeRotation(
          amount, targetAxis, sourceAxis));
      //static Quatf arbitrary(0.0f, 1.0f, 0.0f, 0.0f);
      //Mat4f matArbitrary = Mat4f().storeQuat3dRotation(arbitrary.storeNormalized());
      static int doArbitrary = 0; //1;
      if(doArbitrary > 0) {
        localOffset = matArbitrary.transform(localOffset);
      } else if (doArbitrary < 0) {
        localOffset = matArbitrary.transpose().transform(localOffset);
      } else {
        // do nothing
      }

      static int doLocalTrans = 0;
      Vec4f eyeSpaceOffset = localOffset;
      if(doLocalTrans > 0) {
        eyeSpaceOffset = localEye.transform(eyeSpaceOffset);
      } else if(doLocalTrans < 0) {
        eyeSpaceOffset = localEye.transpose().transform(eyeSpaceOffset);
      } else {
        // do nothing
      }

      static int doCameraTrans = 0; //-1;
      Vec4f worldSpaceOffset = eyeSpaceOffset;
      if(doCameraTrans > 0) {
        worldSpaceOffset = pCamera->_cameraMatrix.transform(worldSpaceOffset);
      } else if(doCameraTrans < 0) {
        worldSpaceOffset = pCamera->_cameraMatrix.transpose().transform(worldSpaceOffset);
      } else {
        // do nothing
      }

      static int doRenderTrans = -1; //0;
      Vec4f renderOffset = worldSpaceOffset;
      if(doRenderTrans > 0) {
        renderOffset = pCamera->_renderMatrix.transform(renderOffset);
      } else if(doRenderTrans < 0) {
        renderOffset = pCamera->_renderMatrix.transpose().transform(renderOffset);
      } else {
        // do nothing
      }

      static float postAmount = -(float)PI / 2.0f;
      static int postTargetAxis = 2;
      static int postSourceAxis = 1;
      Mat4f matPostArbitrary(Mat4f().storeRotation(
          postAmount, postTargetAxis, postSourceAxis));
      //static Quatf arbitrary(0.0f, 1.0f, 0.0f, 0.0f);
      //Mat4f matArbitrary = Mat4f().storeQuat3dRotation(arbitrary.storeNormalized());
      static int doPostArbitrary = 0;
      if(doPostArbitrary > 0) {
        renderOffset = matPostArbitrary.transform(renderOffset);
      } else if (doPostArbitrary < 0) {
        renderOffset = matPostArbitrary.transpose().transform(renderOffset);
      } else {
        // do nothing
      }

      static int finalTransposer[4] = {0, 1, 2, 3};
      static Vec4f finalSign(1.0f, 1.0f, 1.0f, 1.0f);

      Vec4f finalOffset(
          renderOffset[finalTransposer[0]] * finalSign.x,
          renderOffset[finalTransposer[1]] * finalSign.y,
          renderOffset[finalTransposer[2]] * finalSign.z,
          renderOffset[finalTransposer[3]] * finalSign.w);

      const float derpEyes[2] = { -0.6f, 0.6f };
      Vec4f derpEye = (pCamera->_renderMatrix[Camera::RIGHT] * (derpEyes[eye]));

      //pCamera->_renderPos = pCamera->_cameraPos + finalOffset;
      pCamera->UpdateRenderMatrix(&localEye, &finalOffset);

      static int framecount = 0;
      if(eye == 0) {
        framecount++;
      }
      if(framecount > 100) {
        printf("Eye %d\n", eye);
        printf("pristine ovr: \t");   ovrEyePos.printIt();
        printf("\neyespace: \t"); eyeSpaceOffset.printIt();
        printf("\neyecamera: \t"); worldSpaceOffset.printIt();
        printf("\neyerender: \t"); renderOffset.printIt();
        printf("\nfinal: \t"); finalOffset.printIt();
        printf("\nderp: \t"); derpEye.printIt();
        printf("\ncamerapos: \t"); pCamera->_cameraPos.printIt();
        printf("\nlocalEyeMat: \n"); localEye.printIt();
        printf("\ncamera: \n"); pCamera->_cameraMatrix.printIt();
        printf("\nrender: \n"); pCamera->_renderMatrix.printIt();
        printf("\n");
        if(eye != 0) {
          framecount = 0;
        }
      }
    }

    ovrMatrix4f ovrProj = ovrMatrix4f_Projection(m_HMD->DefaultEyeFov[eye],
      0.2f /*zNear*/, 1000.0f /*zFar*/, true /*rightHanded*/);
    pCamera->_zProjectionMatrix.storeFromTransposedArray(&ovrProj.M[0][0]);
  }

  virtual void StartLeftEye(Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
    const int eye = 0;
    StartEye(eye, outRenderColor, outRenderDepth);
    UpdateCameraRenderMatrix(eye, pCamera);
  }

  virtual void StartRightEye(Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
    const int eye = 1;
    StartEye(eye, outRenderColor, outRenderDepth);
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

    if(m_doScreenSaver) {
      float maxDeltaPos = 0.0f;
      float maxDeltaRot = 0.0f;
      for(int i = 0; i < 2; i++) {

        Vec4f oldPos(m_lastEyeRenderPose[i].Position.x, m_lastEyeRenderPose[i].Position.y, m_lastEyeRenderPose[i].Position.z, 0.0f);
        Vec4f newPos(m_eyeRenderPose[i].Position.x, m_eyeRenderPose[i].Position.y, m_eyeRenderPose[i].Position.z, 0.0f);
        maxDeltaPos = max((oldPos - newPos).length(), maxDeltaPos);

        // TODO: do a proper quat slerp dist? This is probably fine actually
        Vec4f oldRot(m_lastEyeRenderPose[i].Orientation.x, m_lastEyeRenderPose[i].Orientation.y, m_lastEyeRenderPose[i].Orientation.z, m_lastEyeRenderPose[i].Orientation.w);
        Vec4f newRot(m_eyeRenderPose[i].Orientation.x, m_eyeRenderPose[i].Orientation.y, m_eyeRenderPose[i].Orientation.z, m_eyeRenderPose[i].Orientation.w);
        maxDeltaRot = max((oldRot - newRot).length(), maxDeltaRot);
      }

      if(maxDeltaPos > s_screenSaverMoveThreshold || maxDeltaRot > s_screenSaverRotateThreshold) {
        m_hadInput = true;
      }

      //printf("VR input:%d pos:%f rot:%f\n", m_hadInput, maxDeltaPos, maxDeltaRot);

    }
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

  virtual std::string GetDeviceName() {
    if(!m_HMD) return std::string("");
    return std::string(m_HMD->DisplayDeviceName);
  }

  virtual bool GetIsDebugDevice() { return m_isDebugDevice; }

  virtual void ToggleFullscreen() {
    if(!m_HMD) return;
    //if(!(m_HMD->HmdCaps & ovrHmdCap_ExtendDesktop)) return;

    m_pWindow->ToggleFullscreenByMonitorName(m_HMD->DisplayDeviceName);
  }

  virtual bool GetPerEyeRenderSize(int& width, int& height) const {
    width = m_eyeRenderWidth;
    height = m_eyeRenderHeight;
    return true;
  }

  virtual bool GetTotalRenderSize(int& width, int& height) const {
    if(!m_HMD) return false;
    width = m_HMD->Resolution.w;
    height = m_HMD->Resolution.h;
    return true;
  }



  virtual void Recenter() {
    if(!m_HMD) return;
    ovrHmd_RecenterPose(m_HMD);
  }
};

VRWrapper* VRWrapper::CreateVR() {
  std::unique_ptr<OVRWrapper> vrWrapper(new OVRWrapper);

  if(!vrWrapper->Initialize()) {
    return NULL;
  }

  return vrWrapper.release();
}
#else //linux_platform
VRWrapper* VRWrapper::CreateVR() {
  return NULL;
}
#endif  //defined(WIN32)


}; // namespace fd
#endif // FD_VR_USE_OCULUS