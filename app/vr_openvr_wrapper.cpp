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
#include "input_handler.h"
#include "render.h"
#include "render_helper.h"
#include "shader.h"
#include "win32_platform.h"
#include "../common/camera.h"
#include "../common/fourmath.h"
#include "../common/tweak.h"

// hmm not great
extern fd::Render g_renderer; // fourd.cpp

namespace fd {

#if defined(WIN32)
  class VVRWrapper : public VRWrapper {
  public:
    PlatformWindow* m_pWindow = NULL;
    vr::IVRSystem* m_HMD = NULL;

    struct EyeRenderInfo
    {
      Mat4f m_eyeViewProj3Pose;
      Texture* m_framebuffer = NULL; // used as a wrappper in rendering
      Texture* m_depthTex = NULL;
      Texture* m_renderTex = NULL;
      Texture* m_resolveTex = NULL;

      EyeRenderInfo() = default;
      ~EyeRenderInfo() {
        delete m_framebuffer;
        delete m_depthTex;
        delete m_renderTex;
        delete m_resolveTex;
      }
    };
    EyeRenderInfo m_eye[2];

    bool m_roomScale = false;
    bool m_flippedEyes = true; // hahahahaha wait seriously?
    vr::TrackedDevicePose_t m_devicePoses[vr::k_unMaxTrackedDeviceCount];
    Pose4f m_device3Poses[vr::k_unMaxTrackedDeviceCount];
    Pose4f m_hmdPose;
    std::string m_strCurrentDeviceClasses;
    char m_rDeviceClassCharCodes[vr::k_unMaxTrackedDeviceCount];
    //vr::ETrackedDeviceClass m_deviceClasses[vr::k_unMaxTrackedDeviceCount];

    unsigned int m_numDistortionIndices = 0;
    GLuint m_unLensVAO = 0;
    GLuint m_glIDVertBuffer = 0;
    GLuint m_glIDIndexBuffer = 0;
    GLuint m_unLensProgramID = 0;
    Shader* m_lensShader = NULL;

    int m_iTrackedControllerCount = 0;
    int m_iTrackedControllerCount_Last = 0;
    int m_iValidPoseCount = 0;
    int m_iValidPoseCount_Last = 0;
    GLuint m_glControllerVertBuffer = 0;
    GLuint m_unControllerVAO = 0;
    unsigned int m_uiControllerVertcount = 0;
    //GLuint m_unControllerAxisTransformProgramID = 0;
    Shader* m_controllerAxis = NULL;
    GLint m_nControllerAxisLocation = -1;
    //GLuint m_unRenderModelProgramID = 0;
    Shader* m_controllerRender = NULL;
	  GLint m_nControllerRenderLocation = -1;

    uint32_t m_eyeRenderWidth = 0;
    uint32_t m_eyeRenderHeight = 0;

    //const Mat4f* m_debugHeadPose;
    //bool m_isDebugDevice
    bool m_debugRendering = false; //true;

  public:
    VVRWrapper() {
      assert(vr::Eye_Left == 0);
      assert(vr::Eye_Right== 1);
      memset(&m_rDeviceClassCharCodes[0], 0, sizeof(m_rDeviceClassCharCodes));
      memset(&m_rTrackedDeviceToRenderModel[0], 0, sizeof(m_rTrackedDeviceToRenderModel));
      m_doScreenSaver = false; // vive has a screensaver already
      m_hadInput = false;
    }

    ~VVRWrapper() {
      for (auto model : m_vecRenderModels) {
        delete model;
      }
      m_vecRenderModels.clear();
      delete m_lensShader;

      if(m_glIDVertBuffer) {
        glDeleteBuffers(1, &m_glIDVertBuffer);
      }
      if(m_glIDIndexBuffer) {
        glDeleteBuffers(1, &m_glIDIndexBuffer);
      }

      if (m_unLensVAO != 0) {
        glDeleteVertexArrays(1, &m_unLensVAO);
      }

      vr::VR_Shutdown();
    }

    bool Initialize() {
      // Loading the SteamVR Runtime
      WasGLErrorPlusPrint();
      vr::EVRInitError eError = vr::VRInitError_None;
      vr::IVRSystem* HMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

      if (eError != vr::VRInitError_None)
      {
        printf("Couldn't init openvr: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return false;
      }

      m_HMD = HMD;
      return !WasGLErrorPlusPrint();
    }

    struct VertexDataLens
    {
      Vec2f position;
      Vec2f texCoordRed;
      Vec2f texCoordGreen;
      Vec2f texCoordBlue;
    };
    
    // Pulled nearly directly from openvr sample
    void SetupDistortion()
    {
      if (!m_HMD)
        return;
      WasGLErrorPlusPrint();

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
      WasGLErrorPlusPrint();
    }

    bool SetupShaders() {
      delete m_lensShader;
      m_lensShader = g_renderer.LoadShader("OpenVrDistortion");
      if(!m_lensShader) {
        printf("Openvr Lens shader failed to load?");
        return false;
      }

      delete m_controllerAxis;
      m_controllerAxis = g_renderer.LoadShader("OpenVrControllerAxis");
      if(!m_controllerAxis) {
        printf("Openvr Controller axis shader failed to load?");
        return false;
      }

      delete m_controllerRender;
      m_controllerRender = g_renderer.LoadShader("OpenVrControllerRender");
      if(!m_controllerRender) {
        printf("Lens shader failed to load?");
        return false;
      }

      return true;
    }

    void RenderDistortion() {
      if (!m_lensShader) return;

      WasGLErrorPlusPrint();
      glDisable(GL_DEPTH_TEST);
      glViewport(0, 0, m_eyeRenderWidth, m_eyeRenderHeight);

      glBindVertexArray(m_unLensVAO);
      m_lensShader->StartUsing();
      //glUseProgram(m_unLensProgramID);

      //render left lens (first half of index array )
      glBindTexture(GL_TEXTURE_2D, m_eye[vr::Eye_Left].m_resolveTex->GetTextureID());
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glDrawElements(GL_TRIANGLES, m_numDistortionIndices / 2, GL_UNSIGNED_SHORT, 0);

      //render right lens (second half of index array )
      glBindTexture(GL_TEXTURE_2D, m_eye[vr::Eye_Right].m_resolveTex->GetTextureID());
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glDrawElements(GL_TRIANGLES, m_numDistortionIndices / 2, GL_UNSIGNED_SHORT, (const void *)(m_numDistortionIndices));

      glBindVertexArray(0);
      m_lensShader->StopUsing();
      //glUseProgram(0);
      WasGLErrorPlusPrint();
    }

  public:
    class CGLRenderModel
    {
    public:
      CGLRenderModel(const std::string & sRenderModelName);
      ~CGLRenderModel();

      bool BInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture);
      void Cleanup();
      void Draw();
      const std::string & GetName() const { return m_sModelName; }

    private:
      GLuint m_glVertBuffer = 0;
      GLuint m_glIndexBuffer = 0;
      GLuint m_glVertArray = 0;
      GLuint m_glTexture = 0;
      GLsizei m_unVertexCount = 0;
      std::string m_sModelName;
    };
    std::vector<CGLRenderModel*> m_vecRenderModels;
    CGLRenderModel* m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
    bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];

    CGLRenderModel* FindOrLoadRenderModel(const char *pchRenderModelName) {
      for(auto model : m_vecRenderModels) {
        if (!_stricmp(model->GetName().c_str(), pchRenderModelName))
          return model;
      }


      CGLRenderModel *pRenderModel = NULL;
      vr::RenderModel_t *pModel;
      vr::EVRRenderModelError error;
      while (1)
      {
        error = vr::VRRenderModels()->LoadRenderModel_Async(pchRenderModelName, &pModel);
        if (error != vr::VRRenderModelError_Loading)
          break;

        Platform::ThreadSleep(1);
      }

      if (error != vr::VRRenderModelError_None)
      {
        printf("Unable to load render model %s - %s\n", pchRenderModelName, vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error));
        return NULL; // move on to the next tracked device
      }

      vr::RenderModel_TextureMap_t *pTexture;
      while (1)
      {
        error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
        if (error != vr::VRRenderModelError_Loading)
          break;

        Platform::ThreadSleep(1);
      }

      if (error != vr::VRRenderModelError_None)
      {
        printf("Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName);
        vr::VRRenderModels()->FreeRenderModel(pModel);
        return NULL; // move on to the next tracked device
      }

      pRenderModel = new CGLRenderModel(pchRenderModelName);
      if (!pRenderModel->BInit(*pModel, *pTexture))
      {
        printf("Unable to create GL model from render model %s\n", pchRenderModelName);
        delete pRenderModel;
        pRenderModel = NULL;
      }
      else
      {
        m_vecRenderModels.push_back(pRenderModel);
      }
      vr::VRRenderModels()->FreeRenderModel(pModel);
      vr::VRRenderModels()->FreeTexture(pTexture);

      return pRenderModel;
    }

    std::string GetTrackedDeviceString(
        vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, 
        vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL) {
      uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
      if (unRequiredBufferLen == 0)
        return "";

      char *pchBuffer = new char[unRequiredBufferLen];
      unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
      std::string sResult = pchBuffer;
      delete[] pchBuffer;
      return sResult;
    }

    // passing in the input handler directly seems crappy
    // I guess there should be a component serving from the inputhandler that provides joysticks?
    // or we just pass in the camera component bus and then when we get openvr events we generate the corresponding input events
    // the downside of that is that the glfw joystick handler will be doing much of the same (presumably) deadzone and binding shit
    // so binding this directly to the joystick handler means there is only one joystick handler and this will write state to a joystick struct
    // which will then be interpreted later this frame to do game input
    // so either there's partially duplicated code or overly complicated abstractions (probably both)
    // the abstraction means less complicated downstream though, which wins
    void HandleInput(float frameTime, InputHandler* input_handler) {
      // Process SteamVR events
      vr::VREvent_t event;
      while (m_HMD->PollNextEvent(&event, sizeof(event))) {
        ProcessVREvent(frameTime, event, input_handler);
      }

      // Process SteamVR controller state
      for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++) {
        vr::VRControllerState_t state;
        if (m_HMD->GetControllerState(unDevice, &state)) {
          m_rbShowTrackedDevice[unDevice] = state.ulButtonPressed == 0;
        }
      }
    }

    void ProcessVREvent(float frameTime, const vr::VREvent_t& event, InputHandler* input_handler) {
      printf("OpenVR event at %f: type:%d\n", frameTime, event.eventType);
      printf("Openvr controller button:%d\n", event.data.controller.button);
      printf("Openvr mouse x:%d y:%d butt:%d\n", event.data.mouse.x, event.data.mouse.y, event.data.mouse.button);

      switch (event.eventType) {
        case vr::VREvent_TrackedDeviceActivated: {
          SetupRenderModelForTrackedDevice(event.trackedDeviceIndex);
          printf("Device %u attached. Setting up render model.\n", event.trackedDeviceIndex);
        } break;
        case vr::VREvent_TrackedDeviceDeactivated: {
          printf("Device %u detached.\n", event.trackedDeviceIndex);
        } break;
        case vr::VREvent_TrackedDeviceUpdated: {
          printf("Device %u updated.\n", event.trackedDeviceIndex);
        } break;
        case vr::VREvent_ButtonPress : {
          switch((vr::EVRButtonId)event.data.controller.button) {
            case vr::k_EButton_Grip :
            case vr::k_EButton_ApplicationMenu :
            case vr::k_EButton_SteamVR_Trigger :
            case vr::k_EButton_SteamVR_Touchpad : {
              input_handler->SendDiscreteSignal("inputShiftSlice", frameTime);
            } break;
          }
        } break;
        case vr::VREvent_TouchPadMove : {
          static TweakVariable touchX("touch mouse x", 0.0f);
          touchX.AsFloat() = event.data.mouse.x;
        } break;
      }
    }

    void SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex) {
      if (unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount)
        return;

      // try to find a model we've already set up
      std::string sRenderModelName = GetTrackedDeviceString(m_HMD, unTrackedDeviceIndex,
          vr::Prop_RenderModelName_String);
      CGLRenderModel *pRenderModel = FindOrLoadRenderModel(sRenderModelName.c_str());
      if (!pRenderModel)
      {
        std::string sTrackingSystemName = GetTrackedDeviceString(m_HMD, unTrackedDeviceIndex, 
            vr::Prop_TrackingSystemName_String);
        printf("Unable to load render model for tracked device %d (%s.%s)",
            unTrackedDeviceIndex, sTrackingSystemName.c_str(), sRenderModelName.c_str());
      }
      else
      {
        m_rTrackedDeviceToRenderModel[unTrackedDeviceIndex] = pRenderModel;
        m_rbShowTrackedDevice[unTrackedDeviceIndex] = true;
      }
    }

    void SetupRenderModels() {
      if (!m_HMD)
        return;
            
      memset(m_rTrackedDeviceToRenderModel, 0, sizeof(m_rTrackedDeviceToRenderModel));

      for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
      {
        if (!m_HMD->IsTrackedDeviceConnected(unTrackedDevice))
          continue;

        SetupRenderModelForTrackedDevice(unTrackedDevice);
      }
      WasGLErrorPlusPrint();
    }

    void UpdateControllerRenderBuffers() {
      // don't draw controllers if somebody else has input focus
      if (m_HMD->IsInputFocusCapturedByAnotherProcess())
        return;

      std::vector<float> vertdataarray;

      m_uiControllerVertcount = 0;
      m_iTrackedControllerCount = 0;

      for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice)
      {
        if (!m_HMD->IsTrackedDeviceConnected(unTrackedDevice))
          continue;

        if (m_HMD->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller)
          continue;

        m_iTrackedControllerCount += 1;

        if (!m_devicePoses[unTrackedDevice].bPoseIsValid)
          continue;

        const Pose4f& mat = m_device3Poses[unTrackedDevice]; //m_rmat4DevicePose[unTrackedDevice];

        Vec4f center = mat.position;

        for (int i = 0; i < 3; ++i)
        {
          Vec4f color(0, 0, 0, 1);
          Vec4f point(0, 0, 0, 0);
          point[i] += 0.05f;  // offset in X, Y, Z
          color[i] = 1.0;  // R, G, B
          point = mat * point;
          vertdataarray.push_back(center.x);
          vertdataarray.push_back(center.y);
          vertdataarray.push_back(center.z);

          vertdataarray.push_back(color.x);
          vertdataarray.push_back(color.y);
          vertdataarray.push_back(color.z);

          vertdataarray.push_back(point.x);
          vertdataarray.push_back(point.y);
          vertdataarray.push_back(point.z);

          vertdataarray.push_back(color.x);
          vertdataarray.push_back(color.y);
          vertdataarray.push_back(color.z);

          m_uiControllerVertcount += 2;
        }

        //Vec4f start = mat * Vec4f(0, 0, -0.02f, 1);
        //Vec4f end = mat * Vec4f(0, 0, -39.f, 1);
        Vec4f start = mat * Vec4f(0, 0, -0.02f, 0);
        Vec4f end = mat * Vec4f(0, 0, -39.f, 0);
        Vec4f color(0.92f, 0.92f, 0.71f, 1.0f);

        vertdataarray.push_back(start.x); vertdataarray.push_back(start.y); vertdataarray.push_back(start.z);
        vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);

        vertdataarray.push_back(end.x); vertdataarray.push_back(end.y); vertdataarray.push_back(end.z);
        vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);
        m_uiControllerVertcount += 2;
      }

      // Setup the VAO the first time through.
      if (m_unControllerVAO == 0)
      {
        glGenVertexArrays(1, &m_unControllerVAO);
        glBindVertexArray(m_unControllerVAO);

        glGenBuffers(1, &m_glControllerVertBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);

        GLuint stride = 2 * 3 * sizeof(float);
        GLuint offset = 0;

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

        offset += sizeof(float) * 3;
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

        glBindVertexArray(0);
      }

      glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);

      // set vertex data if we have some
      if (vertdataarray.size() > 0)
      {
        //$ TODO: Use glBufferSubData for this...
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)* vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW);
      }
      WasGLErrorPlusPrint();
    }

    virtual bool InitializeWindow(PlatformWindow* pWindow, float pixelScale) {
      m_pWindow = pWindow;
      if (!m_HMD)
        return false;

      vr::EVRInitError peError = vr::VRInitError_None;

      if (!vr::VRCompositor()) {
        printf("Compositor initialization failed\n");
        return false;
      }

      m_HMD->GetRecommendedRenderTargetSize(&m_eyeRenderWidth, &m_eyeRenderHeight);

      const float pixelsPerDisplayPixel = pixelScale; //0.25f; // 1.0f;
      bool createSuccess = true;
      for (int e = 0; e < 2; e++) {
        m_eye[e].m_framebuffer = new Texture();

        m_eye[e].m_framebuffer->CreateFrameBuffer();
        glBindFramebuffer(GL_FRAMEBUFFER, m_eye[e].m_framebuffer->m_framebuffer_id);
        WasGLErrorPlusPrint();

        m_eye[e].m_depthTex = new Texture();
        //createSuccess &= m_eye[e].m_depthTex->CreateDepthTarget(
        createSuccess &= m_eye[e].m_depthTex->CreateRenderBuffers(
            m_eyeRenderWidth, m_eyeRenderHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
            m_eye[e].m_depthTex->GetTextureID());
        WasGLErrorPlusPrint();

        m_eye[e].m_renderTex = new Texture();
        createSuccess &= m_eye[e].m_renderTex->CreateColorTarget(
            m_eyeRenderWidth, m_eyeRenderHeight);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
            m_eye[e].m_renderTex->GetTextureID(), 0);
        WasGLErrorPlusPrint();

        m_eye[e].m_resolveTex = new Texture();
        createSuccess &= m_eye[e].m_resolveTex->CreateColorTarget(
            m_eyeRenderWidth, m_eyeRenderHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, m_eye[e].m_resolveTex->m_framebuffer_id);
        WasGLErrorPlusPrint();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
            m_eye[e].m_resolveTex->GetTextureID(), 0);
        WasGLErrorPlusPrint();

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
          printf("ERROR: framebuffer not ready: %d\n", status);
          return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        WasGLErrorPlusPrint();
      }
      if (!createSuccess) {
        printf("VR Render/depth target creation failed\n");
        return false;
      }

      // initialize hand representations
      SetupDistortion();
      if(!SetupShaders())
        return false;
        
      s_Initialized = true;
      
      return true;
    }

    //vvr is right-handed system
    // +y is up
    // +x is to the right
    // -z is going away from you
    //fd is ???-apendaged system
    void ConvertSteamVRMatrixToPose4(
        const vr::HmdMatrix34_t &vrPose, Pose4f& pose) {
      float vals[] = {
        vrPose.m[0][0], vrPose.m[0][1], vrPose.m[0][2], 0.0,
        vrPose.m[1][0], vrPose.m[1][1], vrPose.m[1][2], 0.0,
        vrPose.m[2][0], vrPose.m[2][1], vrPose.m[2][2], 0.0,
        0.0f, 0.0f, 0.0f, 1.0f
      };
      static float signs[] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f
      };
      for(int val = 0; val < (sizeof(signs) / sizeof(signs[0])); val++) {
        vals[val] = signs[val] * vals[val];
      }
      memcpy(pose.rotation.raw(), vals, sizeof(vals));
      static bool transpose = false;
      if(transpose) {
        pose.rotation = pose.rotation.transpose();
      }

      {
        // hacking this to 0 because something is wrong and this is less wrong
        static float worldScale = 0.0f; // ugh, move this to somewhere in camera
        static Vec4f posSigns(1.0f, 1.0f, 1.0f, 1.0f);
        pose.position.set(vrPose.m[0][3], vrPose.m[1][3], vrPose.m[2][3], 0.0f);
        pose.position *= worldScale;
        pose.position *= posSigns;
      }
    }

    void ConvertSteamVRMatrixToMatrix4(
      const vr::HmdMatrix44_t &vrMat, Mat4f& fdMat) {
      float vals[] = {
        vrMat.m[0][0], vrMat.m[1][0], vrMat.m[2][0], vrMat.m[3][0],
        vrMat.m[0][1], vrMat.m[1][1], vrMat.m[2][1], vrMat.m[3][1],
        vrMat.m[0][2], vrMat.m[1][2], vrMat.m[2][2], vrMat.m[3][2],
        vrMat.m[0][3], vrMat.m[1][3], vrMat.m[2][3], vrMat.m[3][3]
      };
      static float signs[] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f
      };
      static bool transpose = false;
      if(transpose) {
        fdMat = fdMat.transpose();
      }
      memcpy(fdMat.raw(), vals, sizeof(vals));
    }

    virtual void StartFrame() {
      WasGLErrorPlusPrint();
      vr::VRCompositor()->WaitGetPoses(
        m_devicePoses, vr::k_unMaxTrackedDeviceCount, NULL, 0);

      for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice) {
        if (m_devicePoses[nDevice].bPoseIsValid) {
          ConvertSteamVRMatrixToPose4(
            m_devicePoses[nDevice].mDeviceToAbsoluteTracking,
            m_device3Poses[nDevice]);
          if (m_rDeviceClassCharCodes[nDevice] == 0) {
            switch (m_HMD->GetTrackedDeviceClass(nDevice)) {
            case vr::TrackedDeviceClass_Controller:        m_rDeviceClassCharCodes[nDevice] = 'C'; break;
            case vr::TrackedDeviceClass_HMD:               m_rDeviceClassCharCodes[nDevice] = 'H'; break;
            case vr::TrackedDeviceClass_Invalid:           m_rDeviceClassCharCodes[nDevice] = 'I'; break;
            case vr::TrackedDeviceClass_Other:             m_rDeviceClassCharCodes[nDevice] = 'O'; break;
            case vr::TrackedDeviceClass_TrackingReference: m_rDeviceClassCharCodes[nDevice] = 'T'; break;
            default:                                       m_rDeviceClassCharCodes[nDevice] = '?'; break;
            }
            m_strCurrentDeviceClasses += m_rDeviceClassCharCodes[nDevice];
          }
          if(m_debugRendering) {
            if(m_HMD->GetTrackedDeviceClass(nDevice) == vr::TrackedDeviceClass_Controller) {
              RenderHelper::RenderAxis(m_device3Poses[nDevice].position, &m_device3Poses[nDevice].rotation, 20 /*scale*/, false /*permanent*/);
              //Pose4f invDev = m_device3Poses[nDevice].invert();
              //RenderHelper::RenderAxis(Vec4f(50.0f, 0.0f, 0.0f, 0.0f) + invDev.position, &invDev.rotation, 20 /*scale*/, false /*permanent*/);
            }
          }
        }
      }

      if (m_devicePoses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
        static bool dropCameraPosition = true;
        if(dropCameraPosition) {
          m_device3Poses[vr::k_unTrackedDeviceIndex_Hmd].position.storeZero();
        }
        static bool doCameraInversion = true;
        if(doCameraInversion) {
          m_hmdPose = m_device3Poses[vr::k_unTrackedDeviceIndex_Hmd].invert();
        } else {
          m_hmdPose = m_device3Poses[vr::k_unTrackedDeviceIndex_Hmd];
        }
        if(m_debugRendering) {
          RenderHelper::RenderAxis(m_hmdPose.position, &m_hmdPose.rotation, 20 /*scale*/, false /*permanent*/);
          RenderHelper::RenderAxis(Vec4f(50.0f, 0.0f, 0.0f, 0.0f) + m_device3Poses[vr::k_unTrackedDeviceIndex_Hmd].position, 
              &m_device3Poses[vr::k_unTrackedDeviceIndex_Hmd].rotation, 20 /*scale*/, false /*permanent*/);
        }
      }
      WasGLErrorPlusPrint();

      UpdateControllerRenderBuffers();

      WasGLErrorPlusPrint();
    }

    void StartEye(vr::Hmd_Eye eye, Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
      WasGLErrorPlusPrint();
      // remove this, it's also set externally, but will be nice for debugging that this vr is on
      glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
      WasGLErrorPlusPrint();
      //glBindFramebuffer( GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId );
      glBindFramebuffer(GL_FRAMEBUFFER, m_eye[eye].m_framebuffer->m_framebuffer_id);
      WasGLErrorPlusPrint();
      glViewport(0, 0, m_eyeRenderWidth, m_eyeRenderHeight);
      WasGLErrorPlusPrint();

      ////  glBindFramebuffer(GL_FRAMEBUFFER, m_eyeRenderTex[eye]->m_framebuffer_id);
      //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      //  GL_TEXTURE_2D, m_eyeRenderTex[eye]->m_texture_id, 0);
      //WasGLErrorPlusPrint();
      //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
      //  GL_TEXTURE_2D, m_eyeDepthTex[eye]->m_texture_id, 0);
      //WasGLErrorPlusPrint();

      if (outRenderColor) {
        *outRenderColor = m_eye[eye].m_renderTex;
      }
      if (outRenderDepth) {
        *outRenderDepth = m_eye[eye].m_depthTex;
      }

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      WasGLErrorPlusPrint();

      DrawControllerModels(eye);

      UpdateCameraRenderMatrix(eye, pCamera);

    }

    void DrawControllerModels(vr::Hmd_Eye eye) {
      bool bIsInputCapturedByAnotherProcess = m_HMD->IsInputFocusCapturedByAnotherProcess();

      if (!bIsInputCapturedByAnotherProcess && m_controllerAxis)
      {
        // draw the controller axis lines
        m_controllerAxis->StartUsing();
        //glUseProgram(m_unControllerTransformProgramID);
        glUniformMatrix4fv(m_nControllerAxisLocation, 1, GL_FALSE, 
          m_eye[eye].m_eyeViewProj3Pose.raw());
        glBindVertexArray(m_unControllerVAO);
        glDrawArrays(GL_LINES, 0, m_uiControllerVertcount);
        glBindVertexArray(0);
        m_controllerAxis->StopUsing();
      }

      if(m_controllerRender) {
        m_controllerRender->StartUsing();
        //glUseProgram(m_unRenderModelProgramID);

        for (uint32_t unTrackedDevice = 0; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
        {
          if (!m_rTrackedDeviceToRenderModel[unTrackedDevice] || !m_rbShowTrackedDevice[unTrackedDevice])
            continue;

          const vr::TrackedDevicePose_t & pose = m_devicePoses[unTrackedDevice];
          if (!pose.bPoseIsValid)
            continue;

          if (bIsInputCapturedByAnotherProcess && m_HMD->GetTrackedDeviceClass(unTrackedDevice) == vr::TrackedDeviceClass_Controller)
            continue;

          const Pose4f& poseDeviceToTracking = m_device3Poses[unTrackedDevice];
          Mat4f matDeviceToTracking = poseDeviceToTracking.projectTo3Pose();
          //Matrix4 matMVP = GetCurrentViewProjectionMatrix(nEye) * matDeviceToTracking;
          Mat4f controllerMatrix = m_eye[eye].m_eyeViewProj3Pose * matDeviceToTracking;
          glUniformMatrix4fv(m_nControllerRenderLocation, 1, GL_FALSE, controllerMatrix.raw());

          m_rTrackedDeviceToRenderModel[unTrackedDevice]->Draw();
        }
        m_controllerRender->StopUsing();
        //glUseProgram(0);
      }
    }

    void FinishEye(int eye, Texture** outRenderColor, Texture** outRenderDepth) {
      WasGLErrorPlusPrint();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      WasFramebufferError();
      WasGLErrorPlusPrint();

      glBindFramebuffer(GL_READ_FRAMEBUFFER, m_eye[eye].m_framebuffer->m_framebuffer_id);
      WasFramebufferError();
      WasGLErrorPlusPrint();
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_eye[eye].m_resolveTex->m_framebuffer_id);
      WasFramebufferError();
      WasGLErrorPlusPrint();

      glBlitFramebuffer(0, 0, m_eyeRenderWidth, m_eyeRenderHeight,
        0, 0, m_eyeRenderWidth, m_eyeRenderHeight,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR);
      WasGLErrorPlusPrint();

      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      WasGLErrorPlusPrint();
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      WasGLErrorPlusPrint();
    }

    //virtual void SetDebugHeadOrientation(const Mat4f* matrix) {
    //  m_debugHeadPose = matrix;
    //}

    Mat4f GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye, Camera* camera) {
      Mat4f fdMat;
      if (!m_HMD)
        return fdMat;

      vr::HmdMatrix44_t vrMat = m_HMD->GetProjectionMatrix(
        nEye, camera->_zNear, camera->_zFar, vr::API_OpenGL);

      ConvertSteamVRMatrixToMatrix4(vrMat, fdMat);

      return fdMat;
    }

    void StoreHMDMatrixPoseEye(vr::Hmd_Eye nEye, Pose4f& outPose) {
      if (!m_HMD)
        return;

      vr::HmdMatrix34_t matEye = m_HMD->GetEyeToHeadTransform(nEye);
      ConvertSteamVRMatrixToPose4(matEye, outPose);

      static bool invertedPose = false; //true;
      if(invertedPose) {
        outPose = outPose.invert();
      }
    }

    // The current intuitive approach is to consider the vr pose as being a 3-pose within the 4-space that is defined as the 4-pose of the camera. To convert from the vr 3-pose to the correct new 4-pose, as defined as already aligned with the camera's xyz-pose out of the 4-pose, transform the vr 3-pose filled out to an identity 4-pose for extra params by the identity matrix, then multiply by the room 4-pose to get the 4-pose of the camera.
    // C is camera 4-pose
    // C * w = C.R * w + C.P, where R is rotation, P is position 
    // C3 is the xyz of C
    // R is the room 4-pose
    // both C and R are relative to world origin, but C is derived per frame from R
    // V is the vr 3-pose
    // for 4-position w
    void UpdateCameraRenderMatrix(vr::Hmd_Eye eye, Camera* pCamera) {
      Pose4f matRoomCameraEye;
      // m_hmdPose should already be an identify padded version of the vr device's coordinates in some space
      Pose4f matEyeToHead;
      StoreHMDMatrixPoseEye(eye, matEyeToHead);

      // ok just do from the origin to start
      matRoomCameraEye = matEyeToHead * m_hmdPose;
      //matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;

      //  const ovrVector3f& localPosOvr = m_eyeDesc[eye].HmdToEyeViewOffset;
      //  Vec4f ovrEyePos(-localPosOvr.x, localPosOvr.y, localPosOvr.z, 0.0f);
      //  ovrEyePos *= worldScale;
      //  ovrEyePos = pCamera->_renderMatrix.transpose().transform(ovrEyePos);
      //  //pCamera->_renderPos = pCamera->_cameraPos + ovrEyePos;
      //  pCamera->UpdateRenderMatrix(&localEye, &ovrEyePos);

      pCamera->UpdateRenderMatrix(&matRoomCameraEye);

      //ovrMatrix4f ovrProj = ovrMatrix4f_Projection(m_HMD->DefaultEyeFov[eye],
      //  0.2f /*zNear*/, 1000.0f /*zFar*/, true /*rightHanded*/);
      pCamera->_zProjectionMatrix = GetHMDMatrixProjectionEye(eye, pCamera);
      
      // TODO: there was quite a lot of code here from before that was involved
      // with getting the transforms right from a previous integration. The new
      // one had similar problems. So, should do transform visualization tools
      // and actually stick with the whole philosophy of the project
    }

    virtual void StartLeftEye(
      Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
      vr::Hmd_Eye eye = vr::Eye_Left;
      if(m_flippedEyes) {
        eye = vr::Eye_Right;
      }
      StartEye(eye, pCamera, outRenderColor, outRenderDepth);
    }

    virtual void FinishLeftEye(
      Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
      vr::Hmd_Eye eye = vr::Eye_Left;
      if(m_flippedEyes) {
        eye = vr::Eye_Right;
      }
      FinishEye(eye, outRenderColor, outRenderDepth);
    }

    virtual void StartRightEye(
      Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
      vr::Hmd_Eye eye = vr::Eye_Right;
      if(m_flippedEyes) {
        eye = vr::Eye_Left;
      }
      StartEye(eye, pCamera, outRenderColor, outRenderDepth);
    }

    virtual void FinishRightEye(
      Camera* pCamera, Texture** outRenderColor, Texture** outRenderDepth) {
      vr::Hmd_Eye eye = vr::Eye_Right;
      if(m_flippedEyes) {
        eye = vr::Eye_Left;
      }
      FinishEye(eye, outRenderColor, outRenderDepth);
    }

    virtual void FinishFrame() {
      WasGLErrorPlusPrint();

      RenderDistortion();
      WasGLErrorPlusPrint();

      vr::Texture_t leftEyeTexture = { (void*)(m_eye[vr::Eye_Left].m_renderTex->GetTextureID()), vr::API_OpenGL, vr::ColorSpace_Linear }; //vr::ColorSpace_Gamma };
      vr::EVRCompositorError err = vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
      if(err != vr::VRCompositorError_None) {
        printf("openvr compositor (left) error: %d\n", err);
      }
      // the compositor is generating opengl error so clear it I guess?
      GLenum droppedError = glGetError();
      static bool didOnce = false;
      if(!didOnce && droppedError != GL_NO_ERROR) {
        const GLubyte *errString = gluErrorString(droppedError);
        printf("openvr compositor (left) OpenGL Error: %s\n", errString);
        didOnce = true;
      }

      WasGLErrorPlusPrint();
      vr::Texture_t rightEyeTexture = { (void*)m_eye[vr::Eye_Right].m_renderTex->GetTextureID(), vr::API_OpenGL, vr::ColorSpace_Linear }; // vr::ColorSpace_Gamma };
      err = vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
      if(err != vr::VRCompositorError_None) {
        printf("openvr compositor (right) error: %d\n", err);
      }
      // the compositor is generating opengl error so clear it I guess?
      GLenum droppedAnotherError = glGetError();
      static bool didAnotherOneOnce = false;
      if(!didAnotherOneOnce && droppedAnotherError != GL_NO_ERROR) {
        const GLubyte *errString = gluErrorString(droppedAnotherError);
        printf("openvr compositor (right) OpenGL Error: %s\n", errString);
        didAnotherOneOnce = true;
      }
      WasGLErrorPlusPrint();

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
      WasGLErrorPlusPrint();
    }

    virtual void SetIsUsingVR(bool usingVR) {
      if (!s_Initialized) return;
      if (s_UsingVR == usingVR) return;

      if (s_UsingVR) {
        s_UsingVR = false;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int restoreWidth;
        int restoreHeight;
        m_pWindow->GetWidthHeight(&restoreWidth, &restoreHeight);
        glViewport(0, 0, restoreWidth, restoreHeight);
      }
      else {
        s_UsingVR = true;
      }
    }

    virtual void SetVRPreferredMovementMode(Camera* pCamera) {
      static bool roomScale = false;
      if(roomScale) {
        pCamera->setMovementMode(Camera::MovementMode::ROOM);
      } else {
        pCamera->setMovementMode(Camera::MovementMode::WALK);
      }
    }

    //virtual std::string GetDeviceName() {
    //  if(!m_HMD) return std::string("");
    //  return std::string(m_HMD->DisplayDeviceName);
    //}

    virtual bool GetIsDebugDevice() { return false; }

    virtual bool GetPerEyeRenderSize(int& width, int& height) const {
      width = m_eyeRenderWidth;
      height = m_eyeRenderHeight;
      return true;
    }

    virtual bool GetTotalRenderSize(int& width, int& height) const {
      if(!m_HMD) return false;
      width = m_eyeRenderWidth;
      height = m_eyeRenderHeight * 2; //uh? dunno?
      return true;
    }

    virtual void Recenter() {
      if(!m_HMD) return;
      m_HMD->ResetSeatedZeroPose();
    }

    ALIGNED_ALLOC_NEW_DEL_OVERRIDE
  };
  

  fd::VVRWrapper::CGLRenderModel::CGLRenderModel(const std::string & sRenderModelName)
    : m_sModelName(sRenderModelName) {
    m_glIndexBuffer = 0;
    m_glVertArray = 0;
    m_glVertBuffer = 0;
    m_glTexture = 0;
  }


  fd::VVRWrapper::CGLRenderModel::~CGLRenderModel() {
    Cleanup();
  }


  //-----------------------------------------------------------------------------
  // Purpose: Allocates and populates the GL resources for a render model
  //-----------------------------------------------------------------------------
  bool fd::VVRWrapper::CGLRenderModel::BInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture) {
    // create and bind a VAO to hold state for this model
    glGenVertexArrays(1, &m_glVertArray);
    glBindVertexArray(m_glVertArray);

    // Populate a vertex buffer
    glGenBuffers(1, &m_glVertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * vrModel.unVertexCount, vrModel.rVertexData, GL_STATIC_DRAW);

    // Identify the components in the vertex buffer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vPosition));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vNormal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, rfTextureCoord));

    // Create and populate the index buffer
    glGenBuffers(1, &m_glIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t)* vrModel.unTriangleCount * 3, vrModel.rIndexData, GL_STATIC_DRAW);

    glBindVertexArray(0);

    // create and populate the texture
    glGenTextures(1, &m_glTexture);
    glBindTexture(GL_TEXTURE_2D, m_glTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight,
      0, GL_RGBA, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData);

    // If this renders black ask McJohn what's wrong.
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

    glBindTexture(GL_TEXTURE_2D, 0);

    m_unVertexCount = vrModel.unTriangleCount * 3;

    return true;
  }


  void fd::VVRWrapper::CGLRenderModel::Cleanup() {
    if (m_glVertBuffer)
    {
      glDeleteBuffers(1, &m_glIndexBuffer);
      glDeleteVertexArrays(1, &m_glVertArray);
      glDeleteBuffers(1, &m_glVertBuffer);
      m_glIndexBuffer = 0;
      m_glVertArray = 0;
      m_glVertBuffer = 0;
    }
  }


  void fd::VVRWrapper::CGLRenderModel::Draw() {
    glBindVertexArray(m_glVertArray);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_glTexture);

    glDrawElements(GL_TRIANGLES, m_unVertexCount, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
  }

  VRWrapper* VRWrapper::CreateVR() {
    std::unique_ptr<VVRWrapper> vrWrapper(new VVRWrapper);

    if (!vrWrapper->Initialize()) {
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
