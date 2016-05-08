#include <math.h>
#include <memory>


#include <GL/glew.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

//#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <stdlib.h>
#include <stdio.h>
#include "../common/fd_simple_file.h"
#include "../common/fourmath.h"
#include "../common/mesh.h"
#include "../common/camera.h"
#include "../common/chunkloader.h"
#include "../common/physics.h"
#include "../common/physics_help.h"
#include "../common/player_capsule_shape.h"
#include "../common/raycast_shape.h"
#include "../common/components/animated_rotation.h"
#include "../common/components/camera_follow.h"
#include "../common/components/periodic_motion.h"
#include "../common/components/physics_component.h"
#include "../common/components/timed_death.h"
#include "../common/thirdparty/argh.h"
#include "entity.h"
#include "imgui_wrapper.h"
#include "input_handler.h"
#include "render.h"
#include "scene.h"
#include "shader.h"
#include "stencil_portal.h"
#include "texture.h"
#include "glhelper.h"
#include "components/gui_input_router.h"
#include "components/reset_watcher.h"
#include "components/quaxol_modifier.h"
#include "components/stencil_portal_component.h"

#define USE_VR
#ifdef USE_VR
#include "vr_wrapper.h"
#endif // USE_VR
#include "platform_interface.h"
#ifdef WIN32
#include "win32_platform.h"
#endif //WIN32

using namespace ::fd;

int _width = 800;
int _height = 600;
float g_blockSize = 10.0f;
Mesh tesseract;
::fd::Scene g_scene;
::fd::Camera g_camera;
::fd::Render g_renderer;
::fd::Shader* g_shader = NULL;
::fd::Entity* g_pointerEntity = NULL;
bool g_captureMouse = false;
bool g_useCapsuleShape = false;
::fd::PlatformWindow* g_platformWindow = NULL;
GLFWwindow* g_glfwWindow = NULL;
::fd::InputHandler g_inputHandler;

::fd::VRWrapper* g_vr = NULL;
Mat4f g_debugHeadPose;
float g_screensaverTime = 0.0f; // 0 is disabled

bool g_startupAddEyeCandy = true;
std::string g_startupLevel = "current.bin";

fd::Shader* LoadShader(const char* shaderName) {
  static std::string g_currentShader;
  std::string shaderDir = "data\\";
  std::string vertPrefix = std::string("vert");
  std::string fragPrefix = std::string("frag");
  std::string ext = std::string(".glsl");

  std::string baseNameWithExt;
  if(shaderName == NULL) {
    std::string search = shaderDir + vertPrefix + "*" + ext;
    const char* currentLevel = (g_currentShader.empty()) ? NULL : g_currentShader.c_str();
    std::string nextNameWithExt;
    if(!::fd::Platform::GetNextFileName(search.c_str(), currentLevel, nextNameWithExt)) {
      return NULL;
    }
    // muhahaha so robust
    const char* baseStart = &(nextNameWithExt.c_str()[4]); // skip vert
    baseNameWithExt.assign(baseStart);
  } else {
    std::string nameBase(shaderName);
    baseNameWithExt = nameBase + ext;
  }

  std::string vertName = shaderDir + vertPrefix + baseNameWithExt;
  std::string fragName = shaderDir + fragPrefix + baseNameWithExt;

  std::unique_ptr<::fd::Shader> shaderMem;
  ::fd::Shader* pShader = ::fd::Shader::GetShaderByRefName(baseNameWithExt);
  if (pShader) {
    pShader->Release();
  } else {
    pShader = new ::fd::Shader();
    shaderMem.reset(pShader);
  }

  pShader->AddDynamicMeshCommonSubShaders();
  if(!pShader->LoadFromFile(baseNameWithExt.c_str(), vertName.c_str(), fragName.c_str())) {
    printf("Failed loading shader!\n");
    return NULL;
  }

  shaderMem.release();
  g_shader = pShader;
  //g_scene.m_pQuaxolShader = g_shader;

  g_currentShader = vertPrefix + baseNameWithExt;
  printf("Loaded shader %s\n", vertName.c_str());

  return pShader;
}

std::string g_levelPath = "data\\levels\\";
bool SaveLevel(const char* levelName) {
  std::string nameExt = ".bin";
  std::string fullname = g_levelPath + std::string(levelName) + nameExt;

  ChunkLoader chunkLoader;
  if(chunkLoader.SaveToFile(fullname.c_str(), g_scene.m_pQuaxolChunk)) {
    printf("Saved out %s\n", fullname.c_str());
    return true;
  }
  return false;
}

static std::string g_lastLevelLoaded;
bool LoadLevel(const char* levelName) {
  Timer timer(std::string("LoadLevel"));

  static std::string g_currentLevel;
  if(levelName) {
    g_lastLevelLoaded.assign(levelName);
  }

  std::string nameExt = ".txt"; // default if unspecified

  std::string baseNameWithExt;
  if(levelName == NULL) {
    std::string search = g_levelPath + "*" + nameExt;
    const char* currentLevel = (g_currentLevel.empty()) ? NULL : g_currentLevel.c_str();
    if(!::fd::Platform::GetNextFileName(search.c_str(), currentLevel, baseNameWithExt)) {
      return false;
    }
  } else {
    std::string nameBase(levelName);
    
    std::string binExt(".bin");
    size_t binExtStart = nameBase.rfind(binExt);
    if(binExtStart != std::string::npos) {
      baseNameWithExt = nameBase;
    } else {
      size_t txtExtStart = nameBase.rfind(nameExt);
      if(txtExtStart != std::string::npos) {
        baseNameWithExt = nameBase;
      } else {
        baseNameWithExt = nameBase + nameExt;
      }
    }
  }
  std::string fullName = g_levelPath + baseNameWithExt;

  ChunkLoader chunkLoader;
  QuaxolChunk* chunk = chunkLoader.LoadFromFile(fullName.c_str());
  if (chunk) {
    chunk->UpdateRendering();
    g_scene.TakeLoadedChunk(chunk);
    //g_scene.AddLoadedChunk(&chunks);
    printf("Level (%s) loaded!\n",
        fullName.c_str());
    printf("Had %d verts and %d tris\n", g_scene.m_pQuaxolChunk->m_verts.size(),
        g_scene.m_pQuaxolChunk->m_indices.size() / 3);

    g_currentLevel = baseNameWithExt;
    return true;
  }
  else {
    printf("Couldn't load the level! name:%s\n", fullName.c_str());
    return false;
  }
}

void ToggleCameraMode(Camera::MovementMode mode) {
  g_camera.setMovementMode(mode);

  if (g_camera.getMovementMode() == Camera::WALK) {
    g_camera.GetComponentBus().SendSignal(
        std::string("DestroyPhysics"), SignalN<>());
    PhysicsShape* shape = NULL;
    if(g_useCapsuleShape) {
      float capsuleRadius = g_blockSize * 0.4f;
      float targetHeight = 10.0f;
      Vec4f offset(0.0f, targetHeight - capsuleRadius, 0.0f, 0.0f);
      shape = new PlayerCapsuleShape(g_scene.m_pPhysics, capsuleRadius, offset);
    } else {
      RaycastShape* rayshape = new RaycastShape(g_scene.m_pPhysics);
      rayshape->AddCapsuleRays(g_blockSize);
      shape = rayshape;
    }
    PhysicsComponent* physicsComp =
        new PhysicsComponent(g_scene.m_pPhysics, shape);
    g_camera.GetComponentBus().AddComponent(physicsComp);
  } else {
    g_camera.GetComponentBus().SendSignal(std::string("DestroyPhysics"), SignalN<>());
  }
}

enum EyeCandyTypes {
  EyeCandyQuad,
  EyeCandyCube,
  EyeCandyTesseract,
  EyeCandy16Cell,
  EyeCandy24Cell,
  EyeCandy120Cell,
  EyeCandy600Cell,
};
typedef std::list<std::unique_ptr<Mesh>> MeshList;

MeshList g_stencilPortalMeshes;
void AddStencilPortals() {
  // 24-26, 16-20, 10-30, 0-4 are plausible bounds for the near arch.
  // 125,120,110-130,100-105 are range from the far portal within the current layout
  Vec4f nearPos(25.0f, 20.0f, 20.0f, 2.0f);
  Vec4f nearExtents(2.0f, 4.0f, 20.0f, 4.0f);
  Mat4f nearOrientation; nearOrientation.storeIdentity();
  Vec4f farPos(125.0f, 120.0f, 120.0f, 100.0f);
  Vec4f farExtents(2.0f, 4.0f, 20.0f, 4.0f);
  Mat4f farOrientation;
  farOrientation.storeRotation((float)PI, Camera::Direction::RIGHT, Camera::Direction::FORWARD);

  // was thinking maybe there should just be portal block types, and it fits within the range.

  
  {
    Entity* newEntity = g_scene.AddEntity();
    newEntity->m_position = farPos;
    newEntity->m_orientation = farOrientation;

    // mem will be owned by entity
    StencilPortalComponent* portalComponent = new StencilPortalComponent();
    *(portalComponent->m_targetPosition) = nearPos;
    *(portalComponent->m_targetOrientation) = nearOrientation;

    g_stencilPortalMeshes.emplace_back(new Mesh());
    Mesh* portalMesh = g_stencilPortalMeshes.back().get();
    portalMesh->buildTesseract(10.0f, Vec4f(0,0,0,0), Vec4f(0,0,0,0));
    newEntity->Initialize(portalMesh, LoadShader("ColorBlendClipped"), NULL);
    newEntity->GetComponentBus().AddComponent(new StencilPortalComponent());
    newEntity->GetComponentBus().AddComponent(portalComponent); // mem now owned by entity
  }

  {
    Entity* newEntity = g_scene.AddEntity();
    newEntity->m_position = nearPos;
    newEntity->m_orientation = nearOrientation;

    // mem will be owned by entity
    StencilPortalComponent* portalComponent = new StencilPortalComponent();
    *(portalComponent->m_targetPosition) = farPos;
    *(portalComponent->m_targetOrientation) = farOrientation;
  
    g_stencilPortalMeshes.emplace_back(new Mesh());
    Mesh* portalMesh = g_stencilPortalMeshes.back().get();
    portalMesh->buildTesseract(10.0f, Vec4f(0,0,0,0), Vec4f(0,0,0,0));
    newEntity->Initialize(portalMesh, LoadShader("ColorBlendClipped"), NULL);
    newEntity->GetComponentBus().AddComponent(portalComponent); // mem now owned by entity
  }
}

MeshList g_eyeCandyMeshes;
void AddEyeCandy(EyeCandyTypes type, const Vec4f& pos) {
  g_eyeCandyMeshes.emplace_back(new Mesh());
  Mesh* candy = g_eyeCandyMeshes.back().get();
  const float size = 30.0f;
  const float smallSize = 15.0f;
  Vec4f smallOff(13.0f, 0.0f, 0.0f, 3.0f);
  switch(type) {
    case EyeCandyQuad:
      candy->buildQuad(smallSize, smallOff, Vec4f());
      break;
    case EyeCandyCube:
      candy->buildCube(smallSize, smallOff, Vec4f());
      break;
    case EyeCandyTesseract:
      candy->buildTesseract(smallSize, smallOff, Vec4f());
      break;
    case EyeCandy16Cell:
      candy->build16cell(size, Vec4f());
      break;
    case EyeCandy24Cell: {
      const float shift = 8.5f;
      const Vec4f offsetSize(shift, shift, 0.5f, shift * 2);
      candy->buildCaylay24Cell(size, offsetSize);
    } break;
    case EyeCandy120Cell: {
      const float shift = 5.5f;
      const Vec4f offsetSize(size / 2 + shift, size / 2 + shift, shift, size / 2 + shift);
      candy->buildCaylay120Cell(size, offsetSize);
    } break;
    case EyeCandy600Cell: {
      const float shift = 5.5f;
      const Vec4f offsetSize(size / 2 + shift, size / 2 + shift, shift, size / 2 + shift);
      candy->buildCaylay600Cell(size, offsetSize);
    } break;
  }

  Entity* pEntity = g_scene.AddEntity();
  // ugh need like a mesh manager and better approach to shader handling
  //pEntity->Initialize(candy, LoadShader("ColorBlend"), NULL);
  pEntity->Initialize(candy, LoadShader("ColorBlendClipped"), NULL);
  pEntity->m_position = pos;
  pEntity->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI * 2.0f, Camera::RIGHT, Camera::INSIDE,
      -20.0f, true));
}

void AddAllEyeCandy() {
  AddStencilPortals();

  // yeah yeah side effects blah blah
  Shader* savedShader = g_shader;

  float closeDist = -50.0f;
  AddEyeCandy(EyeCandyQuad, Vec4f(closeDist, 00.0f, -50.0f, 15.0f));
  AddEyeCandy(EyeCandyCube, Vec4f(closeDist, 00.0f, 00.0f, 15.0f));
  AddEyeCandy(EyeCandyTesseract, Vec4f(closeDist, 00.0f, 50.0f, 5.0f));
  AddEyeCandy(EyeCandy16Cell, Vec4f(-50.0f, 00.0f, 150.0f, 5.0f));
  AddEyeCandy(EyeCandy24Cell, Vec4f(50.0f, 00.0f, 180.0f, 0.0f));
  //AddEyeCandy(EyeCandy120Cell, Vec4f(150.0f, 00.0f, 200.0f, 0.0f));
  //AddEyeCandy(EyeCandy600Cell, Vec4f(250.0f, 00.0f, 240.0f, 0.0f));

  g_shader = savedShader;
  //g_scene.m_pQuaxolShader = g_shader;
}

bool Initialize(int width, int height) {
  //tesseract.buildQuad(10.0f, Vec4f(-20.0, 0, -20.0, 0));
  //tesseract.buildCube(10.0f, Vec4f(0, 0, 0, 0));
  //tesseract.buildTesseract(10.0f, Vec4f(-5.1f,-5.1f,-5.1f,-5.1f), Vec4f(0,0,0,0));
  tesseract.buildTesseract(g_blockSize, Vec4f(0,0,0,0.0f), Vec4f(0,0,0,0));
 
  // Set up some reasonable defaults
  g_camera.SetZProjection(_width, _height, 90.0f /* fov */,
      0.1f /* zNear */, 10000.0f /* zFar */);

  g_renderer.m_multiPass = true; //true; //false;
  if(g_renderer.m_multiPass) {
    g_camera.SetWProjection(
        -50.0f /* wNear */, 50.0f /* wFar */, 0.5f /* wScreenRatio */);
  } else {
    g_camera.SetWProjection(
        0.0f /* wNear */, 50.0f /* wFar */, 0.5f /* wScreenRatio */);
  }

  if(g_useCapsuleShape) {
    g_camera.SetCameraPosition(Vec4f(1.5f, 40.0f, 1.5f, 1.5f));
  } else {
    g_camera.SetCameraPosition(Vec4f(1.5f, 19.5f, 1.5f, 1.5f));
  }
  //g_camera.SetCameraPosition(Vec4f(100.5f, 100.5f, 115.5f, 100.5f));
  //g_camera.ApplyRotationInput(-(float)PI / 1.0f, Camera::FORWARD, Camera::RIGHT);
  g_debugHeadPose.storeIdentity();
  g_camera._yaw = (float)PI;
  ToggleCameraMode(Camera::MovementMode::WALK);

  g_inputHandler.AddInputTarget(&(g_camera.GetComponentBus()));
  g_inputHandler.AddDefaultBindings();
  g_camera.GetComponentBus().AddComponent(new GuiInputRouterComponent());

  if (g_screensaverTime > 0.0f) {
    // reset the game world after 5 minutes of inactivity
    g_camera.GetComponentBus().AddComponent(new ResetWatcherComponent(g_screensaverTime));
  }

  Entity* playerEntity = g_scene.AddEntity();
  playerEntity->GetComponentBus().AddComponent(new CameraFollowComponent(&g_camera));
  playerEntity->GetComponentBus().AddComponent(new QuaxolModifierComponent());
  g_inputHandler.AddInputTarget(&(playerEntity->GetComponentBus()));

  g_camera.MarkStartingPosition();

  //static Vec4f clearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0f);
  glDisable(GL_CULL_FACE); // no backface culling for 4d
  glShadeModel(GL_FLAT);
  //glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //GL_FILL); //GL_LINE);
  //glDisable(GL_MIPMAP);

  if(!g_renderer.Initialize(width, height))
    return false;

  g_renderer.ToggleAlphaDepthModes(Render::AlphaOnDepthOffSrcDest);
  // Just preload the shaders to check for compile errors
  // Last one will be "current"
  if (!LoadShader("AlphaTest")
    || !LoadShader("AlphaTestTex")
    || !LoadShader("ColorBlend")
    || !LoadShader("BlendNoTex")
    || !LoadShader("ColorBlendClipped")
    || !LoadShader("Rainbow")
    ) {
    printf("Shader loading failed\n");
    exit(-1);
  }

  LoadLevel(g_startupLevel.c_str());
  //LoadLevel("current.bin");
  //LoadLevel("4d_double_base");
  //LoadLevel("plus_minus_centered");
  //LoadLevel("pillar");
  g_renderer.AddCamera(&g_camera);
  g_renderer.AddScene(&g_scene);
  g_scene.m_pQuaxolMesh = &tesseract;
  //g_scene.m_pQuaxolShader = g_shader;
  if(!g_scene.Initialize())
    return false;

  {
    Timer texTimer(std::string("texture loading"));
    std::vector<std::string> texList = {
      "data\\textures\\wood.png",
      "data\\textures\\thatch.png",
      "data\\textures\\concrete_brick.png",
      "data\\textures\\orientedTexture.png",
    };

    for (auto texName : texList) {
      std::unique_ptr<::fd::Texture> pTex(new ::fd::Texture());
      if(pTex->LoadFromFile(texName.c_str())) {
        g_scene.AddTexture(pTex.release());
      }
    }

    WasGLErrorPlusPrint();
  }

  if(g_startupAddEyeCandy) {
    AddAllEyeCandy();
  }

  return true;
}

void Deinitialize(void) {
  ::fd::Texture::DeinitializeTextureCache();
  ::fd::Shader::ClearShaderHash();
  ::fd::Platform::Shutdown();
}

// These were nice for debugging a much simpler, stripped down render path
//#define DERP
//#ifndef DERP

void UpdatePerspective() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  g_camera.SetZProjection(_width, _height,
      g_camera._zFov, g_camera._zNear, g_camera._zFar);
}

void SetSimpleProjectiveMode() {
  g_camera.SetWProjection(-8.5f, 8.5f, 1.0f, 1.0f /*animateTime*/);
  //LoadShader("AlphaTest");
  LoadShader("AlphaTestTex");
  g_renderer.ToggleAlphaDepthModes(Render::AlphaTestDepthOnSrcDest);
  UpdatePerspective();
}

void ReshapeGL(GLFWwindow* window, int width, int height) {
  _width = width;
  _height = height;
  glViewport(0, 0, (GLsizei) (_width), (GLsizei) (_height));
  g_renderer.ResizeRenderTargets(width, height);
  UpdatePerspective();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

int mouseX = 0;
int mouseY = 0;
int accumulatedMouseX = 0;
int accumulatedMouseY = 0;

void PassiveMotion(GLFWwindow* window, double dX, double dY) {
  int threshold = 20;
  int x = (int)dX;
  int y = (int)dY;
  int deltaX = x - mouseX;
  int deltaY = y - mouseY;
  mouseX = x;
  mouseY = y;

  //printf("PassiveMotion x:%d y:%d deltaX:%d deltaY:%d accumX:%d accumY:%d\n",
  //  x, y, deltaX, deltaY, accumulatedMouseX, accumulatedMouseY);

  if (deltaX > threshold || deltaY > threshold) {
    //printf("threshold moused\n");
    return;
  }
  accumulatedMouseX += deltaX;
  accumulatedMouseY += deltaY;

  if(g_captureMouse) {
    const int edgeWarp = 50;
    int newX = mouseX;
    int newY = mouseY;
    if(mouseX < edgeWarp || mouseX > (_width - edgeWarp)) {
      newX = _width / 2;
    }
    if(mouseY < edgeWarp || mouseY > (_height - edgeWarp)) {
      newY = _height / 2;
    }
    if(newX != mouseX || newY != mouseY) {
      mouseX = newX;
      mouseY = newY;
      glfwSetCursorPos(g_glfwWindow, newX, newY);
    }
  }
}

enum MouseMoveMode {
  MouseMoveRightUp,
  MouseMoveRightInside,
  MouseMoveInsideUp,
  MouseMoveModeCount,
};
MouseMoveMode g_mouseMoveMode = MouseMoveRightUp;

void ToggleMouseMoveMode() {
  g_mouseMoveMode = (MouseMoveMode)(((int)g_mouseMoveMode + 1) % MouseMoveModeCount);
}

void ApplyMouseMove() {
  Camera::Direction xDir = Camera::RIGHT;
  if(g_mouseMoveMode == MouseMoveInsideUp) {
    xDir = Camera::INSIDE;
  }
  Camera::Direction yDir = Camera::UP;
  if(g_mouseMoveMode == MouseMoveRightInside) {
    yDir = Camera::INSIDE;
  }

  static bool constrainedWalkCamera = true;
  static float moveAmount = 0.01f;
  if (accumulatedMouseX) {
    if(constrainedWalkCamera && g_camera.getMovementMode() == Camera::WALK) {
      g_camera.ApplyYawInput(moveAmount * -accumulatedMouseX);
    } else {
      g_camera.ApplyRotationInput(moveAmount * -accumulatedMouseX, Camera::FORWARD, xDir);
    }
    accumulatedMouseX = 0;
  }

  if (accumulatedMouseY) {
    if(constrainedWalkCamera && g_camera.getMovementMode() == Camera::WALK) {
      g_camera.ApplyPitchInput(moveAmount * accumulatedMouseY);
    } else {
      g_camera.ApplyRotationInput(moveAmount * accumulatedMouseY, Camera::FORWARD, yDir);
    }
    accumulatedMouseY = 0;
  }
}

void DebugRotateVR(float amount, Camera::Direction target, Camera::Direction source) {
  if (g_vr) {
    Mat4f rot;
    rot.storeRotation(amount, target, source);
    g_debugHeadPose = rot * g_debugHeadPose;
    g_vr->SetDebugHeadOrientation(&g_debugHeadPose);
  }
}

void ToggleMouseCapture() {
  g_captureMouse = !g_captureMouse;
  g_platformWindow->CaptureCursor(g_captureMouse);
}

void AddTesseractLineCallback(int x, int y, int z, int w, const Vec4f& pos, const Vec4f& ray) {
  g_scene.m_quaxols.emplace_back(x, y, z, w);

  ////printf("Adding block x:%d y:%d z:%d w:%d\n", x, y, z, w);
  //Entity* pEntity = g_scene.AddEntity();
  //// ugh need like a mesh manager and better approach to shader handling
  //pEntity->Initialize(&tesseract, g_shader, NULL);
  //pEntity->m_orientation.storeIdentity();
  //
  //const float blockSize = 10.0f;
  //Vec4f position(x * blockSize, y * blockSize, z * blockSize, w * blockSize);
  //pEntity->m_position = position;

  ////pEntity->GetComponentBus().AddComponent(
  ////    new AnimatedRotation((float)PI * 10.0f, Camera::RIGHT, Camera::INSIDE,
  ////    20.0f, true));
  //pEntity->GetComponentBus().AddComponent(
  //    new TimedDeath(21.0f /* duration */));
}

void AddRaycastEntity() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getLookForward();
  ray *= 1000.0f;
  float dist;
  if (g_scene.m_pPhysics->RayCast(position, ray, &dist)) {
    Entity* pEntity = g_scene.AddEntity();
    // ugh need like a mesh manager and better approach to shader handling
    
    pEntity->Initialize(&tesseract, LoadShader("ColorBlendClipped"), NULL);
    //pEntity->Initialize(&tesseract, g_shader, NULL);
    //pEntity->m_orientation.storeScale(10.0f);

    pEntity->m_position = position + ray.normalized() * dist;

    pEntity->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI * 2.0f, Camera::RIGHT, Camera::INSIDE,
      -20.0f, true));

    //pEntity->GetComponentBus().AddComponent(
    //    new TimedDeath(10.0f /* duration */));
  }
}

void AddTesseractLine() {
  Vec4f cameraPos = g_camera.getCameraPos();
  cameraPos *= 1.0f / 10.0f;
  Vec4f ray = g_camera.getLookForward();
  ray *= 10.0f;
  DelegateN<void, int, int, int, int, const Vec4f&, const Vec4f&> delegate;
  delegate.Bind(AddTesseractLineCallback);
  g_scene.m_pPhysics->LineDraw4D(cameraPos, ray, delegate);
  
  QuaxolSpec offset(0,0,0,0);
  g_scene.m_pQuaxolChunk->LoadFromList(&(g_scene.m_quaxols), &offset);  
}

void SetIsUsingVR(bool usingVR) {
  if(g_vr) {
    g_vr->SetIsUsingVR(usingVR);
    g_renderer.m_usingVR = usingVR;
  } else {
    g_renderer.m_usingVR = false;
  }
}

void AsciiKeyUpdate(int key, bool isShift);
void Key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    Deinitialize();
    glfwSetWindowShouldClose(window, GL_TRUE);
    return;
  }

  // hacky glut interface follows until a proper rewrite
  if(action != GLFW_PRESS && action != GLFW_REPEAT)
    return;

  bool isShift = (mods & GLFW_MOD_SHIFT);
  int asciiCode = key;
  if(asciiCode >= GLFW_KEY_A && asciiCode <= GLFW_KEY_Z) {
    if(!isShift) {
      // was actually 'a' not 'A'
      asciiCode -= 'A' - 'a';
    }
  }
  if(asciiCode >= GLFW_KEY_0 && asciiCode <= GLFW_KEY_9) {
    if(isShift) {
      // maybe '!' not '1'
      switch(asciiCode) {
        case GLFW_KEY_1: asciiCode = '!'; break;
        case GLFW_KEY_2: asciiCode = '@'; break;
        case GLFW_KEY_3: asciiCode = '#'; break;
        case GLFW_KEY_4: asciiCode = '$'; break;
        case GLFW_KEY_5: asciiCode = '%'; break;
        case GLFW_KEY_6: asciiCode = '^'; break;
        case GLFW_KEY_7: asciiCode = '&'; break;
        case GLFW_KEY_8: asciiCode = '*'; break;
        case GLFW_KEY_9: asciiCode = '('; break;
        case GLFW_KEY_0: asciiCode = ')'; break;
      }
    }
  }

  // code like this last written in the 80s?
  switch(asciiCode) {
    case '=' : asciiCode = (isShift) ? '+' : '='; break;
    case '-' : asciiCode = (isShift) ? '_' : '-'; break;
    case '/' : asciiCode = (isShift) ? '?' : '/'; break;
    case '[' : asciiCode = (isShift) ? '{' : '['; break;
    case ']' : asciiCode = (isShift) ? '}' : ']'; break;
    case '\\' : asciiCode = (isShift) ? '|' : '\\'; break;
    case '\'' : asciiCode = (isShift) ? '"' : '\''; break;
    case ';' : asciiCode = (isShift) ? ':' : ';'; break;
    case ',' : asciiCode = (isShift) ? '<' : ','; break;
    case '.' : asciiCode = (isShift) ? '>' : '.'; break;
    case '`' : asciiCode = (isShift) ? '~' : '`'; break;
  }

  AsciiKeyUpdate(asciiCode, (mods & GLFW_MOD_SHIFT));
}

void AsciiKeyUpdate(int key, bool isShift) {
  static float moveAmount = 1.0f;
  static float rollAmount = moveAmount * 2 * (float)PI / 100.0f;

  switch (key) {
    case '`' : {
      ToggleMouseCapture();
    } break;
    case '~' : {
      ToggleMouseMoveMode();
    } break;
    case '!' : {
      g_renderer.ToggleAlphaDepthModes(Render::EToggleModes);
    } break;
    case '@' : {
      LoadShader("Rainbow");
      UpdatePerspective();
    } break;
    case '#' : {
      LoadShader("ColorBlendClipped");
      UpdatePerspective();
    } break;
    case '$' : {
      LoadShader("AlphaTestTex");
      UpdatePerspective();
    } break;
    case '%' : {
      SetSimpleProjectiveMode();
    } break;
    case '^' : {
      LoadShader("RedShift");
      UpdatePerspective();
    } break;
    case '&' : {
      LoadShader(NULL);
      UpdatePerspective();
    } break;
    case '*' : {
      LoadLevel(NULL);
    } break;
    case '(' : {
      LoadLevel("4d_double_base");
      //LoadLevel("sparse");
      //LoadLevel("single");
    } break;
    case ')' : {
      LoadLevel("current.bin");
    } break;
    case '_' : {
      SaveLevel("current");
    } break;
    case '|' : {
      g_renderer.ToggleMultipassMode(!g_renderer.m_multiPass, _width, _height);
      if(!g_renderer.m_multiPass) {
        g_camera.SetWProjection(0.0f /*near*/, g_camera._wFar, 
            g_camera._wScreenSizeRatio, 1.0f /*animTime*/);
      } else {
        g_camera.SetWProjection(-1.0f * g_camera._wFar, g_camera._wFar, 
            g_camera._wScreenSizeRatio, 1.0f /*animTime*/);
      }
      //if(g_scene.m_pQuaxolChunk) {
      //  g_scene.m_pQuaxolChunk->DebugSwapAxis(2 /*z*/, 1 /*y*/);
      //}
    } break;
    case '+' : {
      static bool fill = false;
      fill = !fill;
      if (fill) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      }
    } break;
    case '=' : {
      AddAllEyeCandy();
    } break;
    case '1' : {
      tesseract.buildQuad(10.0f, Vec4f(0.5, 0.5, 0.5, 0.5), Vec4f(0, 0, 0, 0));
    } break;
    case '2' : {
      tesseract.buildCube(10.0f, Vec4f(0.5, 0.5, 0.5, 0.5), Vec4f(0, 0, 0, 0));
    } break;
    case '3' : {
      tesseract.buildTesseract(10.0f, Vec4f(0.5, 0.5, 0.5, 0.5), Vec4f(0, 0, 0, 0));
    } break;
    case '4' : {
      tesseract.buildFourTetrad(10.0f, Vec4f(0.5, 0.5, 0.5, 0.5));
    } break;
    case '5' : {
      tesseract.buildCaylayEnumerated(30.0f, Vec4f(10.5, 20.5, 0.5, 10.5), 1);
      //tesseract.buildGeneralizedTesseract(10.0f, Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
      //tesseract.buildReferenceTesseract(10.0f, Vec4f(0.5, 0.5, 0.5, 0.5), Vec4f(0, 0, 0, 0));
    } break;
    case '6' : {
      tesseract.buildCaylayEnumerated(30.0f, Vec4f(10.5, 10.5, 0.5, 10.5), -1);
      //tesseract.build120cell(10.0f, Vec4f(0,0,0,0));
      //tesseract.buildCircle(10.0f, Vec4f(10.5, 0.5, 0.5, 0.5), Vec4f(1, 0, 0, 0), Vec4f(0, 1, 0, 0), 6);
      //tesseract.buildCaylayTesseract(10.0f, Vec4f(05.5, 05.5, 05.5, 05.5));
    } break;
    case '7' : {
      tesseract.buildCylinder(10.0f, 10.f, 60);
    } break;
    case '8' : {
      tesseract.buildFourCylinder(10.0f, 10.f, 10.0f, 20);
    } break;
    case '9' : {
      tesseract.build16cell(10.0f, Vec4f(0,0,0,0));
    } break;
    case 27: {
      Deinitialize();
      exit(0);
    } break;
    case ' ' : {
      g_camera.GetComponentBus().SendSignal("AnyInput", SignalN<>());
      g_camera.GetComponentBus().SendSignal("inputJump",
          SignalN<float>(), (float)g_renderer.GetFrameTime());
    } break;
    case 'a' : {
      g_camera.ApplyTranslationInput(-moveAmount, Camera::RIGHT);
    } break;
    case 'd' : {
      g_camera.ApplyTranslationInput(moveAmount, Camera::RIGHT);
    } break;
    case 'w' : {
      g_camera.ApplyTranslationInput(-moveAmount, Camera::FORWARD);
    } break;
    case 's' : {
      g_camera.ApplyTranslationInput(moveAmount, Camera::FORWARD);
    } break;
    case 'q' : {
      g_camera.ApplyTranslationInput(-moveAmount, Camera::UP);
    } break;
    case 'e' : {
      g_camera.ApplyTranslationInput(moveAmount, Camera::UP);
    } break;
    case 'r' : {
      g_camera.ApplyTranslationInput(moveAmount, Camera::INSIDE);
    } break;
    case 'f' : {
      g_camera.ApplyTranslationInput(-moveAmount, Camera::INSIDE);
    } break;
    case 't' : {
      g_camera.ApplyRollInput(-rollAmount, Camera::RIGHT, Camera::UP);
    } break;
    case 'g' : {
      g_camera.ApplyRollInput(rollAmount, Camera::RIGHT, Camera::UP);
    } break;
    case 'y' : {
      g_camera.ApplyRollInput(-rollAmount, Camera::INSIDE, Camera::RIGHT);
    } break;
    case 'h' : {
      g_camera.ApplyRollInput(rollAmount, Camera::INSIDE, Camera::RIGHT);
    } break;
    case 'u' : {
      g_camera.ApplyRollInput(-rollAmount, Camera::UP, Camera::INSIDE);
    } break;
    case 'j' : {
      g_camera.ApplyRollInput(rollAmount, Camera::UP, Camera::INSIDE);
    } break;
    case 'i' : {
      g_camera.GetComponentBus().AddComponent(
          new AnimatedRotation((float)PI * 0.5f,
          (int)::fd::Camera::INSIDE, (int)::fd::Camera::RIGHT,
          2.0f /* duration */, true /* worldSpace */));
    } break;
    case 'k' : {
      g_camera.GetComponentBus().AddComponent(
          new AnimatedRotation(-(float)PI * 0.5f,
          (int)::fd::Camera::INSIDE, (int)::fd::Camera::RIGHT,
          2.0f /* duration */, true /* worldSpace */));
    } break;
    case 'o' : {
      g_camera.GetComponentBus().AddComponent(
          new AnimatedRotation((float)PI * 0.5f,
          (int)::fd::Camera::UP, (int)::fd::Camera::INSIDE,
          2.0f /* duration */, true /* worldSpace */));
    } break;
    case 'l' : {
      g_camera.GetComponentBus().AddComponent(
          new AnimatedRotation(-(float)PI * 0.5f,
          (int)::fd::Camera::UP, (int)::fd::Camera::INSIDE,
          2.0f /* duration */, true /* worldSpace */));
    } break;
    // Sure this looks like an unsorted mess, but is spatially aligned kinda.
    case 'x' : {
      g_camera.SetWProjection(
          g_camera._wNear - 1.0f, g_camera._wFar, g_camera._wScreenSizeRatio, 1.0f /*animateTime*/);
    } break;
    case 'c' : {
      g_camera.SetWProjection(
          g_camera._wNear + 1.0f, g_camera._wFar, g_camera._wScreenSizeRatio, 1.0f /*animateTime*/);
    } break;
    case 'v' : {
      g_camera.SetWProjection(
          g_camera._wNear, g_camera._wFar - 1.0f, g_camera._wScreenSizeRatio, 1.0f /*animateTime*/);
    } break;
    case 'b' : {
      g_camera.SetWProjection(
          g_camera._wNear, g_camera._wFar + 1.0f, g_camera._wScreenSizeRatio, 1.0f /*animateTime*/);
    } break;
    case 'n' : {
      g_camera.SetWProjection(
          g_camera._wNear, g_camera._wFar, g_camera._wScreenSizeRatio - 0.1f, 1.0f /*animateTime*/);
    } break;
    case 'm' : {
      g_camera.SetWProjection(
          g_camera._wNear, g_camera._wFar, g_camera._wScreenSizeRatio + 0.1f, 1.0f /*animateTime*/);
    } break;
    case '?' : {
      g_camera.printIt();
    } break;
    case '\'' : {
      tesseract.printIt();
    } break;
    case '"' : {
      Shader::ReloadAllShaders();
    } break;
    case '[' : {
      if (g_camera.getMovementMode() == Camera::LOOK) {
        ToggleCameraMode(Camera::WALK);
      } else {
        ToggleCameraMode(Camera::LOOK);
      }
    } break;
    case ']' : { // ortho projection
      static float savedWScreenRatio = 0.5f;
      float newWScreenRatio = savedWScreenRatio;
      if (g_camera._wScreenSizeRatio == 1.0f) {
        g_camera.SetWProjection(
            g_camera._wNear, g_camera._wFar, savedWScreenRatio, 1.0f /*animateTime*/);
      } else {
        savedWScreenRatio = g_camera._wScreenSizeRatio;
        g_camera.SetWProjection(
            g_camera._wNear, g_camera._wFar, 1.0f, 1.0f /*animateTime*/);
      }
    } break;
    case 'z' : {
      g_inputHandler.DoCommand("inputAddQuaxol", g_renderer.GetFrameTimeF());
    } break;
    case 'S' : {
      AddRaycastEntity();
    } break;
    case 'X' : {
      AddTesseractLine();
    } break;
    case 'Z' : {
      g_inputHandler.DoCommand("inputRemoveQuaxol", g_renderer.GetFrameTimeF());
    } break;
    case 'C' : {
      if (g_vr) {
        g_vr->Recenter();
      }
    } break;
    case 'V' : {
      SetIsUsingVR(!g_renderer.m_usingVR);
    } break;
    case 'F' : {
      if (g_vr) {
        g_vr->ToggleFullscreen();
      }
    } break;
    case 'I' : {
      DebugRotateVR((float)PI / 32.0f, Camera::UP, Camera::FORWARD);
    } break;
    case 'K' : {
      DebugRotateVR(-(float)PI / 32.0f, Camera::UP, Camera::FORWARD);
    } break;
    case 'J' : {
      DebugRotateVR((float)PI / 32.0f, Camera::RIGHT, Camera::FORWARD);
    } break;
    case 'L' : {
      DebugRotateVR(-(float)PI / 32.0f, Camera::RIGHT, Camera::FORWARD);
    } break;
  }
}

void Draw(GLFWwindow* window) {
  if (!g_shader)
    return;

  //g_renderer.m_clearColor = Vec4f(158.0f / 255.0f, 224.0f / 255.0f, 238.0f / 255.0f, 0.0f);
  g_renderer.m_clearColor = Vec4f(0, 0, 0, 0);
  glClearColor(g_renderer.m_clearColor.x, g_renderer.m_clearColor.y, g_renderer.m_clearColor.z, g_renderer.m_clearColor.w);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  float frameTime = (float)g_renderer.GetFrameTime();
  static bool renderVRUI = true;

  if(VRWrapper::IsUsingVR() && g_vr) {
    Vec2f uiOffset(150.0f, 400.0f);
    g_vr->StartFrame();
    Texture* renderColor;
    Texture* renderDepth;
    glClearColor(g_renderer.m_clearColor.x, g_renderer.m_clearColor.y, g_renderer.m_clearColor.z, g_renderer.m_clearColor.w);
    g_vr->StartLeftEye(&g_camera, &renderColor, &renderDepth);
    g_renderer.RenderAllScenesPerCamera(renderColor, renderDepth);
    if(renderVRUI)
      ImGuiWrapper::Render(frameTime, uiOffset, &g_renderer, true /*doUpdate*/);
    glClearColor(g_renderer.m_clearColor.x, g_renderer.m_clearColor.y, g_renderer.m_clearColor.z, g_renderer.m_clearColor.w);
    g_vr->StartRightEye(&g_camera, &renderColor, &renderDepth);
    g_renderer.RenderAllScenesPerCamera(renderColor, renderDepth);
    if(renderVRUI)
      ImGuiWrapper::Render(frameTime, uiOffset, &g_renderer, false /*doUpdate*/);
    g_vr->FinishFrame();
  } else {
    Vec2f uiOffset(0, 0);
    g_camera.UpdateRenderMatrix(NULL /*lookOffset*/, NULL /*posOffset*/);
    glClearColor(g_renderer.m_clearColor.x, g_renderer.m_clearColor.y, g_renderer.m_clearColor.z, g_renderer.m_clearColor.w);
    g_renderer.RenderAllScenesPerCamera(NULL /*renderColor*/, NULL /*renderDepth*/);
    ImGuiWrapper::Render(frameTime, uiOffset, &g_renderer, true /*doUpdate*/);
  }

  if(ImGuiWrapper::s_bGuiDisabled) {
    static int framecounter = 0;
    if(framecounter++ % 100 == 0) {
      printf("FPS %f\n", 1.0f / frameTime);
    }
  }


  glFlush();
  glfwSwapBuffers(window);
}

void RaycastToOpenQuaxol() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getLookForward();
  ray *= 1000.0f;

  Vec4f hitPos(0.0f, -1000.0f, 0.0f, 0.0f);
  static Entity* openEntity = NULL;
  if (g_scene.m_pPhysics->RayCastToOpenQuaxol(
      position, ray, NULL /*quaxolSpec*/, &hitPos)) {
    if(!openEntity) {
      openEntity = g_scene.AddEntity();
      static Mesh addBlock;
      addBlock.buildTesseract(g_blockSize, Vec4f(), Vec4f());
      openEntity->Initialize(&addBlock, g_shader, NULL);
    }
    
    openEntity->m_position = hitPos;
    openEntity->m_pShader = g_shader;
  } else {
    if(openEntity) {
      openEntity->m_pShader = NULL;      
    }
  }
  
}

void RaycastToCollsion() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getLookForward();
  ray *= 1000.0f;
  float dist;

  Vec4f hitPos(0.0f, -1000.0f, 0.0f, 0.0f);

  if (g_scene.m_pPhysics->RayCast(position, ray, &dist)) {
    hitPos = position + ray.normalized() * dist;
  }
  
  if(!g_pointerEntity) {
    g_pointerEntity = g_scene.AddEntity();
    g_pointerEntity->Initialize(&tesseract, g_shader, NULL);
    g_pointerEntity->m_orientation.storeScale(0.1f);
    g_pointerEntity->m_position = hitPos;

    g_pointerEntity->GetComponentBus().AddComponent(
        new AnimatedRotation((float)PI * 10.0f, Camera::RIGHT, Camera::INSIDE,
        -20.0f, true));
  } else {
    g_pointerEntity->m_position = hitPos;
    g_pointerEntity->m_pShader = g_shader;
  }
}

void UpdatePointerEntity() {
  return;
  //if(g_camera.getMovementMode() == Camera::LOOK)
  {
    static bool quaxolMode = true;
    if(quaxolMode) {
      RaycastToOpenQuaxol();
    } else {
      RaycastToCollsion();
    }
  }
}

void StepFrame() {
  g_renderer.UpdateFrameTime();

  if(g_camera._restartedGame) {
    g_camera._restartedGame = false;
    std::string levelName = g_lastLevelLoaded;
    LoadLevel(levelName.c_str());
  }
  
  int guiWidth = 0; // 0 means ImGui will figure it out
  int guiHeight = 0; 
  if(g_vr && g_vr->IsUsingVR()) {
    g_vr->GetPerEyeRenderSize(guiWidth, guiHeight);
  }
  ImGuiWrapper::NewFrame((float)g_renderer.GetFrameTime(), guiWidth, guiHeight);
  
  g_inputHandler.PollJoysticks();
  g_inputHandler.ApplyJoystickInput((float)g_renderer.GetFrameTime());
  ApplyMouseMove();
  
  if(g_vr && g_vr->IsUsingVR() && g_vr->m_doScreenSaver) {
    if(g_vr->m_hadInput) {
      g_inputHandler.SendAnyInputSignal(&(g_camera.GetComponentBus()));
    }
  }

  g_renderer.Step();
  g_scene.Step((float)g_renderer.GetFrameTime());

  UpdatePointerEntity();
}

void StaticInitialize() {
  Timer staticTimer(std::string("StaticInitialize"));

  QuaxolChunk::BuildCanonicalCubesByDir(g_blockSize);
  QuaxolChunk::BuildCanonicalCubesByFlag(g_blockSize);
}

float Rand() { return (float)rand() / (float)RAND_MAX; }
const float cfThreshold = 0.000001f;
bool IsEqual(float l, float r) { return (fabs(l - r) < cfThreshold); }
bool IsZero(float val) { return (fabs(val) < cfThreshold); }

// TODO: tests in here is totally tacky, move them.
void RunTests() {
//return;

  Vec4f a(Rand(), Rand(), Rand(), Rand());
  Vec4f b(Rand(), Rand(), Rand(), Rand());
  Vec4f c(Rand(), Rand(), Rand(), Rand());
  Vec4f d = a.cross(b, c);
  assert(IsZero(a.dot(d)));
  assert(IsZero(b.dot(d)));
  assert(IsZero(c.dot(d)));

  Mat4f look(a,b,c,d);
  look.storeOrthognoal(0, 1, 2, 3);
  assert(IsEqual(look[0].length(), 1.0f));
  assert(IsEqual(look[1].length(), 1.0f));
  assert(IsEqual(look[2].length(), 1.0f));
  assert(IsEqual(look[3].length(), 1.0f));
  assert(IsZero(look[0].dot(look[1])));
  assert(IsZero(look[0].dot(look[2])));
  assert(IsZero(look[0].dot(look[3])));
  assert(IsZero(look[1].dot(look[2])));
  assert(IsZero(look[1].dot(look[3])));
  assert(IsZero(look[2].dot(look[3])));

  Mat4f iden;
  iden.storeIdentity();
  assert(iden == (iden * iden));

  Mat4f scale(look);
  Mat4f invScale = scale.inverse();
  assert(!(scale == invScale));
  assert(scale == invScale.inverse());

  Mat4f rotXFourth;
  rotXFourth.storeRotation((float)PI / 2.0, 1, 0);
  Mat4f rotXEighth;
  rotXEighth.storeRotation((float)PI / 4.0, 1, 0);
  assert(iden == (rotXFourth * rotXFourth * rotXFourth * rotXFourth * iden));
  assert(rotXEighth * rotXEighth == rotXFourth);
  assert(!(rotXFourth * rotXFourth == rotXFourth));

  Quatf rotPiDiv4(sqrt(2.0f) / 2.0f, sqrt(2.0f) / 2.0f, 0.0f, 0.0f);
  Quatf rotPiDiv2(0.0f, 1.0f, 0.0f, 0.0f);
  assert(rotPiDiv2.approxEqual(rotPiDiv4 * rotPiDiv4, 0.00001f));

  //Mat4f threeMat;
  //threeMat.eigen().

  Shader::RunTests();
  Camera::RunTests();
  Physics::RunTests();
  PhysicsHelp::RunTests();
}

void glfwErrorCallback(int error, const char* description) {
  printf("GLFW Error: %d :  %s\n", error, description);
}

// Soooo tacky!
#define RUN_TESTS

int main(int argc, const char *argv[]) {

  std::string keepAliveFileName("");
  bool displayUsage = false;
  float pixelScale = 0.5f; //1.0f;
  float screenSaverMoveThreshold = 0.00003f;
  float screenSaverRotateThreshold = 0.0001f;
  
  argh::Argh cmd_line;
  cmd_line.addFlag(displayUsage,
      "--help", "Display help (you probably figured this one out)");
  cmd_line.addFlag(displayUsage, 
      "-h", "Display help (you probably figured this one out)");
  cmd_line.addFlag(displayUsage, 
      "-?", "Display help (you probably figured this one out)");
  cmd_line.addFlag(ImGuiWrapper::s_bGuiDisabled, 
      "--disable_ui", "Disable the gui, useful for when it sucks");
  cmd_line.addOption<float>(pixelScale, pixelScale, 
      "--pixel_scale", "How much to reduce the render target to improve fill rate");
  cmd_line.addOption<std::string>(g_startupLevel, g_startupLevel, 
      "--start_level", "Level name to start with, without path, with extension"); 
  cmd_line.addFlag(g_startupAddEyeCandy, 
      "--add_eye_candy", "Adds a bunch of shapes on startup");
  cmd_line.addOption<std::string>(keepAliveFileName, keepAliveFileName, 
      "--keep_alive_file_name", 
      "Will write to this file every 5 seconds to indicate the process is alive"); 
  cmd_line.addOption<float>(g_screensaverTime, g_screensaverTime, 
      "--screensaver", "Enable the screensaver system");
  cmd_line.addOption<float>(VRWrapper::s_screenSaverMoveThreshold, VRWrapper::s_screenSaverMoveThreshold, 
      "--screensaver_move_thresh", "How much VR head movement turns off the screensaver");
  cmd_line.addOption<float>(VRWrapper::s_screenSaverRotateThreshold, VRWrapper::s_screenSaverRotateThreshold,
      "--screensaver_rotate_thresh", "How much VR head rotation turns off the screensaver");
  cmd_line.parse(argc, argv);
  
  printf("Screensaver was %f\n", g_screensaverTime);
  printf("DisabledUI was %d\n", ImGuiWrapper::s_bGuiDisabled);

  if(displayUsage) {
    printf("Helpy?\n%s\n", cmd_line.getUsage().c_str());
    return 0;
  }

  printf("keep alive is %s\n", keepAliveFileName.c_str());

  StaticInitialize();

#ifdef RUN_TESTS
  RunTests();
#endif // RUN_TESTS

  // ovr is supposed to preceed glfw
  g_vr = VRWrapper::CreateVR();
  if(g_screensaverTime > 0.0f && g_vr) {
    g_vr->m_doScreenSaver = true;
  } 

  glfwSetErrorCallback(glfwErrorCallback);

  if(!glfwInit()) {
    printf("glfw init fail\n");
    return -1;
  }

  int startWidth = 640;
  int startHeight = 580;
  GLFWmonitor* monitor = NULL;
  const GLFWvidmode* vidMode;
  static bool startFullscreen = false;
  static bool startVRFullscreen = true;
  if(startVRFullscreen && g_vr && !g_vr->GetIsDebugDevice()) {
    std::string deviceName = g_vr->GetDeviceName();
    monitor = PlatformWindow::GetRiftMonitorByName(deviceName.c_str());
    vidMode = glfwGetVideoMode(monitor);
    g_vr->GetTotalRenderSize(startWidth, startHeight);
  } else if(startFullscreen) {
    //startVRFullscreen = false;
    monitor = glfwGetPrimaryMonitor();
    vidMode = glfwGetVideoMode(monitor);
    startWidth = vidMode->width;
    startHeight = vidMode->height;
  } else {
    startFullscreen = false;
    startVRFullscreen = false;
    monitor = glfwGetPrimaryMonitor();
    vidMode = glfwGetVideoMode(monitor);
  }

  glfwWindowHint(GLFW_RED_BITS, vidMode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, vidMode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, vidMode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, vidMode->refreshRate);
  if(!startFullscreen && !startVRFullscreen) {
    monitor = NULL;
  }

  char windowTitle[] = "fourd";
  g_glfwWindow = glfwCreateWindow(
      startWidth, startHeight, windowTitle, monitor, NULL /*share*/);
  if(!g_glfwWindow) {
    printf("glfwCreateWindow fail\n");
    return -1;
  }

  glfwMakeContextCurrent(g_glfwWindow);
  glfwSwapInterval(0); //1);

  glfwSetFramebufferSizeCallback(g_glfwWindow, ReshapeGL);

  g_platformWindow = ::fd::Platform::Init(windowTitle, startWidth, startHeight);
  g_platformWindow->m_glfwWindow = g_glfwWindow; // some days it's like fuck it, let's do it the wrong way
  
  glewExperimental=TRUE;
  GLenum err;
  if((err = glewInit()) != GLEW_OK) {
    printf("Glew init fail: Error: %s\n", glewGetErrorString(err));
    return -1;
  }

  if(!ImGuiWrapper::Init(g_glfwWindow, Key, NULL /*mouseButtonCallback*/))
  {
    printf("Imgui init fail\n");
    return -1;
  }
  //glfwSetKeyCallback(g_glfwWindow, Key); // If not using ImGui, put this back.
  
  g_vr->InitializeWindow(g_platformWindow, pixelScale);

  glfwSetCursorPosCallback(g_glfwWindow, PassiveMotion);

  glfwSetWindowRefreshCallback(g_glfwWindow, Draw);

  if(g_vr && startVRFullscreen) {
    SetIsUsingVR(true);
    glfwSetInputMode(g_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }
  
  if(!Initialize(startWidth, startHeight)) {
    printf("Initialized failed\n");
    return -1;
  }

  ReshapeGL(g_glfwWindow, startWidth, startHeight);

  double keepAliveTime = 5.0f; //write every five seconds
  double keepAliveNext = 0.0f;

  while(true) {
    StepFrame();
    Draw(g_glfwWindow);

    if(!keepAliveFileName.empty()) {
      if(g_renderer.GetTotalTime() >= keepAliveNext) {
        keepAliveNext = g_renderer.GetTotalTime() + keepAliveTime; 
        fd_file_write_vec(keepAliveFileName.c_str(), std::vector<unsigned char>('!'));
      }
    }

    glfwPollEvents();
    if(glfwWindowShouldClose(g_glfwWindow)) {
      break;
    }
  }

  ImGuiWrapper::Shutdown();
  glfwTerminate();

  delete g_vr;

  return 0;
}


//#else // DERP
// contents of the derp render path have been moved to fourd.derp.cpp.bak

//#endif // DERP
