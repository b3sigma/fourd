// Heavily borrowing from openvr hellovr sample integration
#include "vr_wrapper.h"
#ifdef FD_VR_USE_OPENVR

#include <openvr.h>
#include <memory>

#include "texture.h"

//#ifdef WIN32
//#include <Windows.h>
//#endif
#include "glhelper.h"
#include "render.h"
#include "win32_platform.h"
#include "../common/fourmath.h"
#include "../common/camera.h"

namespace fd {

#if defined(WIN32)
class VVRWrapper : public VRWrapper {
public:
  PlatformWindow* m_pWindow;
  vr::IVRSystem* m_HMD;
  
  Texture* m_eyeRenderTex[2];
  Texture* m_eyeDepthTex[2];
  Texture* m_eyeResolveTex[2];

  vr::TrackedDevicePose_t m_devicePoses[vr::k_unMaxTrackedDeviceCount];
  Pose4f m_device3Poses[vr::k_unMaxTrackedDeviceCount];
  Pose4f m_hmdPose;
  std::string m_strCurrentDeviceClasses;
  char m_rDeviceClassCharCodes[vr::k_unMaxTrackedDeviceCount];
	//vr::ETrackedDeviceClass m_deviceClasses[vr::k_unMaxTrackedDeviceCount];

  unsigned int m_numDistortionIndices;
	GLuint m_unLensVAO;
	GLuint m_glIDVertBuffer;
	GLuint m_glIDIndexBuffer;
  GLuint m_unLensProgramID;

  uint32_t m_eyeRenderWidth;
  uint32_t m_eyeRenderHeight;

  //const Mat4f* m_debugHeadPose;
  //bool m_isDebugDevice

  VVRWrapper() : m_HMD(NULL), m_pWindow(NULL)
      , m_eyeRenderWidth(0), m_eyeRenderHeight(0)
      , m_numDistortionIndices(0)
  //    , m_debugHeadPose(NULL)
  //    , m_isDebugDevice(true)
  {
    memset(&m_rDeviceClassCharCodes[0], 0, sizeof(m_rDeviceClassCharCodes));
    for(int e = 0; e < 2; e++) {
      m_eyeRenderTex[e] = NULL;
      m_eyeDepthTex[e] = NULL;
      m_eyeResolveTex[e] = NULL;
    }
    m_doScreenSaver = false; // so, this may not be the way we want to do this for vive, as they have screensaver already it seems like, and it's better.
    m_hadInput = false;
  }

  ~VVRWrapper() {
    for(int e = 0; e < 2; e++) {
      delete m_eyeRenderTex[e];
      delete m_eyeDepthTex[e];
      delete m_eyeResolveTex[e];
    }
    vr::VR_Shutdown();
  }

  bool Initialize() {
    // Loading the SteamVR Runtime
	  vr::EVRInitError eError = vr::VRInitError_None;
	  vr::IVRSystem* HMD = vr::VR_Init( &eError, vr::VRApplication_Scene );

	  if ( eError != vr::VRInitError_None )
	  {
      printf("Couldn't init openvr: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		  return false;
	  }

    m_HMD = HMD;
	  return true;
  }

  //bool SetupTrackedDeviceModels() {
  //  return false;
  //}

  struct VertexDataLens
  {
    Vec2f position;
    Vec2f texCoordRed;
    Vec2f texCoordGreen;
    Vec2f texCoordBlue;
  };

  void SetupDistortion()
  {
    if (!m_HMD)
      return;

    GLushort m_iLensGridSegmentCountH = 43;
    GLushort m_iLensGridSegmentCountV = 43;

    float w = (float)(1.0 / float(m_iLensGridSegmentCountH - 1));
    float h = (float)(1.0 / float(m_iLensGridSegmentCountV - 1));

    float u, v = 0;

    std::vector<VertexDataLens> vVerts(0);
    VertexDataLens vert;

    //left eye distortion verts
    float Xoffset = -1;
    for (int y = 0; y < m_iLensGridSegmentCountV; y++)
    {
      for (int x = 0; x < m_iLensGridSegmentCountH; x++)
      {
        u = x*w; v = 1 - y*h;
        vert.position = Vec2f(Xoffset + u, -1 + 2 * y*h);

        vr::DistortionCoordinates_t dc0 = m_HMD->ComputeDistortion(vr::Eye_Left, u, v);

        vert.texCoordRed = Vec2f(dc0.rfRed[0], 1 - dc0.rfRed[1]);
        vert.texCoordGreen = Vec2f(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
        vert.texCoordBlue = Vec2f(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

        vVerts.push_back(vert);
      }
    }

    //right eye distortion verts
    Xoffset = 0;
    for (int y = 0; y < m_iLensGridSegmentCountV; y++)
    {
      for (int x = 0; x < m_iLensGridSegmentCountH; x++)
      {
        u = x*w; v = 1 - y*h;
        vert.position = Vec2f(Xoffset + u, -1 + 2 * y*h);

        vr::DistortionCoordinates_t dc0 = m_HMD->ComputeDistortion(vr::Eye_Right, u, v);

        vert.texCoordRed = Vec2f(dc0.rfRed[0], 1 - dc0.rfRed[1]);
        vert.texCoordGreen = Vec2f(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
        vert.texCoordBlue = Vec2f(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

        vVerts.push_back(vert);
      }
    }

    std::vector<GLushort> vIndices;
    GLushort a, b, c, d;

    GLushort offset = 0;
    for (GLushort y = 0; y < m_iLensGridSegmentCountV - 1; y++)
    {
      for (GLushort x = 0; x < m_iLensGridSegmentCountH - 1; x++)
      {
        a = m_iLensGridSegmentCountH*y + x + offset;
        b = m_iLensGridSegmentCountH*y + x + 1 + offset;
        c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
        d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
        vIndices.push_back(a);
        vIndices.push_back(b);
        vIndices.push_back(c);

        vIndices.push_back(a);
        vIndices.push_back(c);
        vIndices.push_back(d);
      }
    }

    offset = (m_iLensGridSegmentCountH)*(m_iLensGridSegmentCountV);
    for (GLushort y = 0; y < m_iLensGridSegmentCountV - 1; y++)
    {
      for (GLushort x = 0; x < m_iLensGridSegmentCountH - 1; x++)
      {
        a = m_iLensGridSegmentCountH*y + x + offset;
        b = m_iLensGridSegmentCountH*y + x + 1 + offset;
        c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
        d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
        vIndices.push_back(a);
        vIndices.push_back(b);
        vIndices.push_back(c);

        vIndices.push_back(a);
        vIndices.push_back(c);
        vIndices.push_back(d);
      }
    }
    m_numDistortionIndices = vIndices.size();

    glGenVertexArrays(1, &m_unLensVAO);
    glBindVertexArray(m_unLensVAO);

    glGenBuffers(1, &m_glIDVertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_glIDVertBuffer);
    glBufferData(GL_ARRAY_BUFFER, vVerts.size()*sizeof(VertexDataLens), &vVerts[0], GL_STATIC_DRAW);

    glGenBuffers(1, &m_glIDIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIDIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size()*sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordRed));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordGreen));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordBlue));

    glBindVertexArray(0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  void RenderDistortion()
  {
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, m_eyeRenderWidth, m_eyeRenderHeight);

    glBindVertexArray(m_unLensVAO);
    glUseProgram(m_unLensProgramID);

    //render left lens (first half of index array )
    glBindTexture(GL_TEXTURE_2D, m_eyeResolveTex[0]->GetTextureID());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glDrawElements(GL_TRIANGLES, m_numDistortionIndices / 2, GL_UNSIGNED_SHORT, 0);

    //render right lens (second half of index array )
    glBindTexture(GL_TEXTURE_2D, m_eyeResolveTex[0]->GetTextureID());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glDrawElements(GL_TRIANGLES, m_numDistortionIndices / 2, GL_UNSIGNED_SHORT, (const void *)(m_numDistortionIndices));

    glBindVertexArray(0);
    glUseProgram(0);
  }

  // need cleanup for distortion

  virtual bool InitializeWindow(PlatformWindow* pWindow, float pixelScale) {
    m_pWindow = pWindow;
	  if(!m_HMD)
		  return false;

	  vr::EVRInitError peError = vr::VRInitError_None;

	  if (!vr::VRCompositor())
	  {
		  printf("Compositor initialization failed\n");
		  return false;
	  } 

	  m_HMD->GetRecommendedRenderTargetSize(&m_eyeRenderWidth, &m_eyeRenderHeight);

    const float pixelsPerDisplayPixel = pixelScale; //0.25f; // 1.0f;
    bool createSuccess = true;
    for (int e = 0; e < 2; e++) {
      m_eyeRenderTex[e] = new Texture();
      createSuccess &= m_eyeRenderTex[e]->CreateRenderTarget(
          m_eyeRenderWidth, m_eyeRenderHeight);
      m_eyeDepthTex[e] = new Texture();
      createSuccess &= m_eyeDepthTex[e]->CreateDepthTarget(
          m_eyeRenderWidth, m_eyeRenderHeight);
    }
    if(!createSuccess) {
      printf("VR Render/depth target creation failed\n");
      return false;
    }
    
    // initialize hand representations
    // initialize vertex buffer for distortion mesh



  //  ovrGLConfig config;
  //  config.OGL.Header.API = ovrRenderAPI_OpenGL;
  //  config.OGL.Header.BackBufferSize = m_HMD->Resolution;
  //  config.OGL.Header.Multisample = 0;
  //  config.OGL.Window = m_pWindow->m_hWnd;

  //  int ovrDistortionCaps = ovrDistortionCap_Vignette
  //      | ovrDistortionCap_Chromatic
  //      | ovrDistortionCap_TimeWarp
  //      | ovrDistortionCap_Overdrive;
  //  ovrHmd_ConfigureRendering(m_HMD, &config.Config, ovrDistortionCaps,
  //      m_HMD->DefaultEyeFov, m_eyeDesc);

  //  ovrHmd_AttachToWindow(m_HMD, m_pWindow->m_hWnd,
  //      NULL /*destRect*/, NULL /*srcRect*/);
  //  ovrHmd_DismissHSWDisplay(m_HMD);

    s_Initialized = true;
    //s_UsingVR = true;

    return true;
  }

  //vvr is right-handed system
  // +y is up
  // +x is to the right
  // -z is going away from you
  //fd is ???-apendaged system
  void ConvertSteamVRMatrixToMatrix4(
      const vr::HmdMatrix34_t &vrPose, Pose4f& pose)
  {
    // probably still totally wrong, look here first when it's messed
    float vals[] = {    
      vrPose.m[0][0], vrPose.m[1][0], vrPose.m[2][0], 0.0,
      vrPose.m[0][1], vrPose.m[1][1], vrPose.m[2][1], 0.0,
      vrPose.m[0][2], vrPose.m[1][2], vrPose.m[2][2], 0.0,
      0.0f, 0.0f, 0.0f, 1.0f
    };
    memcpy(pose.rotation.raw(), vals, sizeof(vals));
    pose.position.set(vrPose.m[0][3], vrPose.m[1][3], vrPose.m[2][3], 0.0f);
  }

  virtual void StartFrame() {
    vr::VRCompositor()->WaitGetPoses(
      m_devicePoses, vr::k_unMaxTrackedDeviceCount, NULL, 0);

    for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
    {
      if (m_devicePoses[nDevice].bPoseIsValid)
      {
        ConvertSteamVRMatrixToMatrix4(m_devicePoses[nDevice].mDeviceToAbsoluteTracking,
          m_device3Poses[nDevice]);
        if (m_rDeviceClassCharCodes[nDevice] == 0)
        {
          switch (m_HMD->GetTrackedDeviceClass(nDevice))
          {
          case vr::TrackedDeviceClass_Controller:        m_rDeviceClassCharCodes[nDevice] = 'C'; break;
          case vr::TrackedDeviceClass_HMD:               m_rDeviceClassCharCodes[nDevice] = 'H'; break;
          case vr::TrackedDeviceClass_Invalid:           m_rDeviceClassCharCodes[nDevice] = 'I'; break;
          case vr::TrackedDeviceClass_Other:             m_rDeviceClassCharCodes[nDevice] = 'O'; break;
          case vr::TrackedDeviceClass_TrackingReference: m_rDeviceClassCharCodes[nDevice] = 'T'; break;
          default:                                       m_rDeviceClassCharCodes[nDevice] = '?'; break;
          }
        }
        m_strCurrentDeviceClasses += m_rDeviceClassCharCodes[nDevice];
      }
    }

    if (m_devicePoses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
    {
      m_hmdPose = m_device3Poses[vr::k_unTrackedDeviceIndex_Hmd].invert();
    }


//  ovrHmd_BeginFrame(m_HMD, 0);

  //  ovrVector3f viewOffsets[2] = {
  //      m_eyeDesc[0].HmdToEyeViewOffset,
  //      m_eyeDesc[1].HmdToEyeViewOffset };

  //  if(m_doScreenSaver) {
  //    // reset screensaver detection state
  //    m_lastEyeRenderPose[0] = m_eyeRenderPose[0];
  //    m_lastEyeRenderPose[1] = m_eyeRenderPose[1];
  //    m_hadInput = false;
  //  }
  //  ovrHmd_GetEyePoses(m_HMD, 0, viewOffsets, m_eyeRenderPose,
  //      NULL /*trackingState*/);

  }

  void StartEye(int eye, Texture** outRenderColor, Texture** outRenderDepth) {
  //  glBindFramebuffer(GL_FRAMEBUFFER, m_eyeRenderTex[eye]->m_framebuffer_id);
  //  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
  //      GL_TEXTURE_2D, m_eyeRenderTex[eye]->m_texture_id, 0);
  //  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
  //      GL_TEXTURE_2D, m_eyeDepthTex[eye]->m_texture_id, 0);

  //  if(outRenderColor) {
  //    *outRenderColor = m_eyeRenderTex[eye];
  //  }
  //  if(outRenderDepth) {
  //    *outRenderDepth = m_eyeDepthTex[eye];
  //  }

  //  glViewport(0, 0,
  //      enderTex[eye]->m_width, m_eyeRenderTex[eye]->m_height);
  //  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }


  //virtual void SetDebugHeadOrientation(const Mat4f* matrix) {
  //  m_debugHeadPose = matrix;
  //}

  //void UpdateCameraRenderMatrix(int eye, Camera* pCamera) {

  //  const ovrQuatf& localQuatOvr = m_eyeRenderPose[eye].Orientation;
  //  Quatf localEyeQuat(localQuatOvr.w, localQuatOvr.x, localQuatOvr.y, localQuatOvr.z);
  //  Mat4f localEye;
  //  localEye.storeQuat3dRotation(localEyeQuat);

  //  if(m_debugHeadPose) {
  //    localEye = *m_debugHeadPose;
  //  }

  //  const float worldScale = 20.0f;
  //  static bool simpleEyeOffset = true;
  //  if(simpleEyeOffset) {
  //    const ovrVector3f& localPosOvr = m_eyeDesc[eye].HmdToEyeViewOffset;
  //    Vec4f ovrEyePos(-localPosOvr.x, localPosOvr.y, localPosOvr.z, 0.0f);
  //    ovrEyePos *= worldScale;
  //    ovrEyePos = pCamera->_renderMatrix.transpose().transform(ovrEyePos);
  //    //pCamera->_renderPos = pCamera->_cameraPos + ovrEyePos;
  //    pCamera->UpdateRenderMatrix(&localEye, &ovrEyePos);

  //  } else {
  //    const ovrVector3f& ovrEyeZero = m_eyeRenderPose[0].Position;
  //    const ovrVector3f& ovrEyeOne = m_eyeRenderPose[1].Position;

  //    Vec4f ovrEyeDifference(
  //        ovrEyeOne.x - ovrEyeZero.x,
  //        ovrEyeOne.y - ovrEyeZero.y,
  //        ovrEyeOne.z - ovrEyeZero.z,
  //        0.0f);

  //    const ovrVector3f& localPosOvr = m_eyeRenderPose[eye].Position;
  //    static int transposer[3] = {0, 1, 2};
  //    //static Vec4f sign(1.0f, 1.0f, 1.0f, 1.0f);
  //    static Vec4f sign(-1.0f, 1.0f, 1.0f, 1.0f);
  //    Vec4f ovrEyePos(localPosOvr.x, localPosOvr.y, localPosOvr.z, 0.0f);

  //    // TODO: try to convert ovr into local using simple sign switch
  //    // then independently do eye and forehead transformations
  //    Vec4f ovrForehead;
  //    if(eye == 0) {
  //      ovrForehead = ovrEyePos - (ovrEyeDifference * 0.5f);
  //    } else {
  //      ovrForehead = ovrEyePos - (ovrEyeDifference * -0.5f);
  //    }

  //    ovrEyePos *= worldScale;
  //    Vec4f localOffset(
  //        ovrEyePos[transposer[0]] * sign.x,
  //        ovrEyePos[transposer[1]] * sign.y,
  //        ovrEyePos[transposer[2]] * sign.z,
  //        0.0f);

  //    static float amount = (float)PI / 2.0f;
  //    static int targetAxis = 2;
  //    static int sourceAxis = 1;
  //    Mat4f matArbitrary(Mat4f().storeRotation(
  //        amount, targetAxis, sourceAxis));
  //    //static Quatf arbitrary(0.0f, 1.0f, 0.0f, 0.0f);
  //    //Mat4f matArbitrary = Mat4f().storeQuat3dRotation(arbitrary.storeNormalized());
  //    static int doArbitrary = 0; //1;
  //    if(doArbitrary > 0) {
  //      localOffset = matArbitrary.transform(localOffset);
  //    } else if (doArbitrary < 0) {
  //      localOffset = matArbitrary.transpose().transform(localOffset);
  //    } else {
  //      // do nothing
  //    }

  //    static int doLocalTrans = 0;
  //    Vec4f eyeSpaceOffset = localOffset;
  //    if(doLocalTrans > 0) {
  //      eyeSpaceOffset = localEye.transform(eyeSpaceOffset);
  //    } else if(doLocalTrans < 0) {
  //      eyeSpaceOffset = localEye.transpose().transform(eyeSpaceOffset);
  //    } else {
  //      // do nothing
  //    }

  //    static int doCameraTrans = 0; //-1;
  //    Vec4f worldSpaceOffset = eyeSpaceOffset;
  //    if(doCameraTrans > 0) {
  //      worldSpaceOffset = pCamera->_cameraMatrix.transform(worldSpaceOffset);
  //    } else if(doCameraTrans < 0) {
  //      worldSpaceOffset = pCamera->_cameraMatrix.transpose().transform(worldSpaceOffset);
  //    } else {
  //      // do nothing
  //    }

  //    static int doRenderTrans = -1; //0;
  //    Vec4f renderOffset = worldSpaceOffset;
  //    if(doRenderTrans > 0) {
  //      renderOffset = pCamera->_renderMatrix.transform(renderOffset);
  //    } else if(doRenderTrans < 0) {
  //      renderOffset = pCamera->_renderMatrix.transpose().transform(renderOffset);
  //    } else {
  //      // do nothing
  //    }

  //    static float postAmount = -(float)PI / 2.0f;
  //    static int postTargetAxis = 2;
  //    static int postSourceAxis = 1;
  //    Mat4f matPostArbitrary(Mat4f().storeRotation(
  //        postAmount, postTargetAxis, postSourceAxis));
  //    //static Quatf arbitrary(0.0f, 1.0f, 0.0f, 0.0f);
  //    //Mat4f matArbitrary = Mat4f().storeQuat3dRotation(arbitrary.storeNormalized());
  //    static int doPostArbitrary = 0;
  //    if(doPostArbitrary > 0) {
  //      renderOffset = matPostArbitrary.transform(renderOffset);
  //    } else if (doPostArbitrary < 0) {
  //      renderOffset = matPostArbitrary.transpose().transform(renderOffset);
  //    } else {
  //      // do nothing
  //    }

  //    static int finalTransposer[4] = {0, 1, 2, 3};
  //    static Vec4f finalSign(1.0f, 1.0f, 1.0f, 1.0f);

  //    Vec4f finalOffset(
  //        renderOffset[finalTransposer[0]] * finalSign.x,
  //        renderOffset[finalTransposer[1]] * finalSign.y,
  //        renderOffset[finalTransposer[2]] * finalSign.z,
  //        renderOffset[finalTransposer[3]] * finalSign.w);

  //    const float derpEyes[2] = { -0.6f, 0.6f };
  //    Vec4f derpEye = (pCamera->_renderMatrix[Camera::RIGHT] * (derpEyes[eye]));

  //    //pCamera->_renderPos = pCamera->_cameraPos + finalOffset;
  //    pCamera->UpdateRenderMatrix(&localEye, &finalOffset);

  //    static int framecount = 0;
  //    if(eye == 0) {
  //      framecount++;
  //    }
  //    if(framecount > 100) {
  //      printf("Eye %d\n", eye);
  //      printf("pristine ovr: \t");   ovrEyePos.printIt();
  //      printf("\neyespace: \t"); eyeSpaceOffset.printIt();
  //      printf("\neyecamera: \t"); worldSpaceOffset.printIt();
  //      printf("\neyerender: \t"); renderOffset.printIt();
  //      printf("\nfinal: \t"); finalOffset.printIt();
  //      printf("\nderp: \t"); derpEye.printIt();
  //      printf("\ncamerapos: \t"); pCamera->_cameraPos.printIt();
  //      printf("\nlocalEyeMat: \n"); localEye.printIt();
  //      printf("\ncamera: \n"); pCamera->_cameraMatrix.printIt();
  //      printf("\nrender: \n"); pCamera->_renderMatrix.printIt();
  //      printf("\n");
  //      if(eye != 0) {
  //        framecount = 0;
  //      }
  //    }
  //  }

  //  ovrMatrix4f ovrProj = ovrMatrix4f_Projection(m_HMD->DefaultEyeFov[eye],
  //    0.2f /*zNear*/, 1000.0f /*zFar*/, true /*rightHanded*/);
  //  pCamera->_zProjectionMatrix.storeFromTransposedArray(&ovrProj.M[0][0]);
  //}

  //virtual void StartLeftEye(Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
  //  const int eye = 0;
  //  StartEye(eye, outRenderColor, outRenderDepth);
  //  UpdateCameraRenderMatrix(eye, pCamera);
  //}

  //virtual void StartRightEye(Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
  //  const int eye = 1;
  //  StartEye(eye, outRenderColor, outRenderDepth);
  //  UpdateCameraRenderMatrix(eye, pCamera);
  //}

  virtual void FinishFrame() {
    RenderDistortion();
    
		vr::Texture_t leftEyeTexture = {(void*)m_eyeResolveTex[0]->GetTextureID(), vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture );
		vr::Texture_t rightEyeTexture = {(void*)m_eyeResolveTex[1]->GetTextureID(), vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture );

  //  if(m_doScreenSaver) {
  //    float maxDeltaPos = 0.0f;
  //    float maxDeltaRot = 0.0f;
  //    for(int i = 0; i < 2; i++) {

  //      Vec4f oldPos(m_lastEyeRenderPose[i].Position.x, m_lastEyeRenderPose[i].Position.y, m_lastEyeRenderPose[i].Position.z, 0.0f);
  //      Vec4f newPos(m_eyeRenderPose[i].Position.x, m_eyeRenderPose[i].Position.y, m_eyeRenderPose[i].Position.z, 0.0f);
  //      maxDeltaPos = max((oldPos - newPos).length(), maxDeltaPos);

  //      // TODO: do a proper quat slerp dist? This is probably fine actually
  //      Vec4f oldRot(m_lastEyeRenderPose[i].Orientation.x, m_lastEyeRenderPose[i].Orientation.y, m_lastEyeRenderPose[i].Orientation.z, m_lastEyeRenderPose[i].Orientation.w);
  //      Vec4f newRot(m_eyeRenderPose[i].Orientation.x, m_eyeRenderPose[i].Orientation.y, m_eyeRenderPose[i].Orientation.z, m_eyeRenderPose[i].Orientation.w);
  //      maxDeltaRot = max((oldRot - newRot).length(), maxDeltaRot);
  //    }

  //    if(maxDeltaPos > s_screenSaverMoveThreshold || maxDeltaRot > s_screenSaverRotateThreshold) {
  //      m_hadInput = true;
  //    }

  //    //printf("VR input:%d pos:%f rot:%f\n", m_hadInput, maxDeltaPos, maxDeltaRot);

  //  }

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

  //virtual std::string GetDeviceName() {
  //  if(!m_HMD) return std::string("");
  //  return std::string(m_HMD->DisplayDeviceName);
  //}

  //virtual bool GetIsDebugDevice() { return m_isDebugDevice; }

  //virtual void ToggleFullscreen() {
  //  if(!m_HMD) return;
  //  //if(!(m_HMD->HmdCaps & ovrHmdCap_ExtendDesktop)) return;

  //  m_pWindow->ToggleFullscreenByMonitorName(m_HMD->DisplayDeviceName);
  //}

  //virtual bool GetPerEyeRenderSize(int& width, int& height) const {
  //  width = m_eyeRenderWidth;
  //  height = m_eyeRenderHeight;
  //  return true;
  //}

  //virtual bool GetTotalRenderSize(int& width, int& height) const {
  //  if(!m_HMD) return false;
  //  width = m_HMD->Resolution.w;
  //  height = m_HMD->Resolution.h;
  //  return true;
  //}



  //virtual void Recenter() {
  //  if(!m_HMD) return;
  //  ovrHmd_RecenterPose(m_HMD);
  //}

  ALIGNED_ALLOC_NEW_DEL_OVERRIDE
};

VRWrapper* VRWrapper::CreateVR() {
  std::unique_ptr<VVRWrapper> vrWrapper(new VVRWrapper);

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
#endif //def FD_VR_USE_OPENVR
