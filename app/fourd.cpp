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
#include "../common/fourmath.h"
#include "../common/mesh.h"
#include "../common/camera.h"
#include "../common/chunkloader.h"
#include "../common/physics.h"
#include "../common/physics_help.h"
#include "../common/player_capsule_shape.h"
#include "../common/raycast_shape.h"
#include "../common/components/animated_rotation.h"
#include "../common/components/periodic_motion.h"
#include "../common/components/physics_component.h"
#include "../common/components/timed_death.h"
#include "entity.h"
#include "imgui_wrapper.h"
#include "input_handler.h"
#include "render.h"
#include "scene.h"
#include "shader.h"
#include "texture.h"
#include "glhelper.h"

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
::fd::PlatformWindow* g_platformWindow = NULL;
GLFWwindow* g_glfwWindow = NULL;
::fd::InputHandler g_inputHandler;

::fd::VRWrapper* g_vr = NULL;
Mat4f g_debugHeadPose;

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
  ::fd::Shader* pShader = ::fd::Shader::GetShaderByRefName(baseNameWithExt.c_str());
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
  g_scene.m_pQuaxolShader = g_shader;

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

bool LoadLevel(const char* levelName) {
  Timer timer(std::string("LoadLevel"));

  static std::string g_currentLevel;

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
      baseNameWithExt = nameBase + nameExt;
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
    static bool usePlayerShape = false;
    PhysicsShape* shape = NULL;
    if(usePlayerShape) {
      shape = new PlayerCapsuleShape(g_scene.m_pPhysics, g_blockSize);
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
};
typedef std::list<std::unique_ptr<Mesh>> MeshList;
MeshList g_eyeCandyMeshes;

void AddEyeCandy(EyeCandyTypes type, const Vec4f& pos) {
  g_eyeCandyMeshes.emplace_back(new Mesh());
  Mesh* candy = g_eyeCandyMeshes.back().get();
  const float size = 30.0f;
  switch(type) {
    case EyeCandyQuad:
      candy->buildQuad(size, Vec4f(), Vec4f());
      break;
    case EyeCandyCube:
      candy->buildCube(size, Vec4f(), Vec4f());
      break;
    case EyeCandyTesseract:
      candy->buildTesseract(size, Vec4f(), Vec4f());
      break;
    case EyeCandy16Cell:
      candy->build16cell(size, Vec4f());
      break;
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

void AddEyeCandy() {

  // yeah yeah side effects blah blah
  Shader* savedShader = g_shader;

  AddEyeCandy(EyeCandyQuad, Vec4f(-50.0f, 00.0f, -50.0f, 0.0f));
  AddEyeCandy(EyeCandyCube, Vec4f(-50.0f, 00.0f, 00.0f, 0.0f));
  AddEyeCandy(EyeCandyTesseract, Vec4f(-50.0f, 00.0f, 50.0f, 0.0f));
  AddEyeCandy(EyeCandy16Cell, Vec4f(-50.0f, 00.0f, 150.0f, 0.0f));

  g_shader = savedShader;
  g_scene.m_pQuaxolShader = g_shader;
}

bool Initialize(int width, int height) {
  //tesseract.buildQuad(10.0f, Vec4f(-20.0, 0, -20.0, 0));
  //tesseract.buildCube(10.0f, Vec4f(0, 0, 0, 0));
  //tesseract.buildTesseract(10.0f, Vec4f(-5.1f,-5.1f,-5.1f,-5.1f), Vec4f(0,0,0,0));
  tesseract.buildTesseract(g_blockSize, Vec4f(0,0,0,0.0f), Vec4f(0,0,0,0));
 
  // Set up some reasonable defaults
  g_camera.SetZProjection(_width, _height, 90.0f /* fov */,
      0.1f /* zNear */, 10000.0f /* zFar */);

  g_renderer.m_multiPass = false; //true; //false;
  if(g_renderer.m_multiPass) {
    g_camera.SetWProjection(
        -50.0f /* wNear */, 50.0f /* wFar */, 0.5f /* wScreenRatio */);
  } else {
    g_camera.SetWProjection(
        0.0f /* wNear */, 40.0f /* wFar */, 0.5f /* wScreenRatio */);
  }

  g_camera.SetCameraPosition(Vec4f(1.5f, 20.5f, 1.5f, 1.5f));
  //g_camera.SetCameraPosition(Vec4f(100.5f, 100.5f, 115.5f, 100.5f));
  //g_camera.ApplyRotationInput(-(float)PI / 1.0f, Camera::FORWARD, Camera::RIGHT);
  g_debugHeadPose.storeIdentity();
  g_camera._yaw = (float)PI;
  ToggleCameraMode(Camera::MovementMode::WALK);

  g_inputHandler.m_inputTarget = &(g_camera.GetComponentBus());
  g_inputHandler.AddDefaultBindings();

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

  LoadLevel("current.bin");
  //LoadLevel("4d_double_base");
  //LoadLevel("plus_minus_centered");
  //LoadLevel("pillar");
  g_renderer.AddCamera(&g_camera);
  g_renderer.AddScene(&g_scene);
  g_scene.m_pQuaxolMesh = &tesseract;
  g_scene.m_pQuaxolShader = g_shader;
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

  AddEyeCandy();

  return true;
}

void Deinitialize(void) {
  ::fd::Texture::DeinitializeTextureCache();
  ::fd::Shader::ClearShaderHash();
  ::fd::Platform::Shutdown();
}

//#define DERP
#ifndef DERP

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

void AddQuaxolUnderCursor() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getLookForward();
  ray *= 1000.0f;

  QuaxolSpec gridPos;
  if(g_scene.m_pPhysics->RayCastToOpenQuaxol(
      position, ray, &gridPos, NULL /*hitPos*/)) {
    g_scene.SetQuaxolAt(gridPos, true /*present*/);
    printf("Added quaxol at x:%d y:%d z:%d w:%d\n",
        gridPos.x, gridPos.y, gridPos.z, gridPos.w);
  }
}

void RemoveQuaxolUnderCursor() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getLookForward();
  ray *= 1000.0f;

  QuaxolSpec gridPos;
  if(g_scene.m_pPhysics->RayCastToPresentQuaxol(
      position, ray, &gridPos, NULL /*hitPos*/)) {
    g_scene.SetQuaxolAt(gridPos, false /*present*/);
    printf("Removed quaxol at x:%d y:%d z:%d w:%d\n",
        gridPos.x, gridPos.y, gridPos.z, gridPos.w);
  }
}

void AddRaycastEntity() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getLookForward();
  ray *= 1000.0f;
  float dist;
  if (g_scene.m_pPhysics->RayCast(position, ray, &dist)) {
    Entity* pEntity = g_scene.AddEntity();
    // ugh need like a mesh manager and better approach to shader handling
    pEntity->Initialize(&tesseract, g_shader, NULL);
    pEntity->m_orientation.storeScale(0.1f);

    pEntity->m_position = position + ray.normalized() * dist;

    pEntity->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI * 10.0f, Camera::RIGHT, Camera::INSIDE,
      20.0f, true));

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

  switch(asciiCode) {
    case '=' : asciiCode = (isShift) ? '+' : '='; break;
    case '-' : asciiCode = (isShift) ? '_' : '-'; break;
    case '/' : asciiCode = (isShift) ? '?' : '/'; break;
    case '[' : asciiCode = (isShift) ? '{' : '['; break;
    case ']' : asciiCode = (isShift) ? '}' : ']'; break;
    case '\\' : asciiCode = (isShift) ? '|' : '\\'; break;
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
      AddEyeCandy();
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
      tesseract.buildGeneralizedTesseract(10.0f, Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
      //tesseract.buildReferenceTesseract(10.0f, Vec4f(0.5, 0.5, 0.5, 0.5), Vec4f(0, 0, 0, 0));
    } break;
    case '6' : {
      //tesseract.build120cell(10.0f, Vec4f(0,0,0,0));
      tesseract.buildCircle(10.0f, Vec4f(10.5, 0.5, 0.5, 0.5), Vec4f(1, 0, 0, 0), Vec4f(0, 1, 0, 0), 6);
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
      //AddRaycastEntity();
      AddQuaxolUnderCursor();
    } break;
    case 'X' : {
      AddTesseractLine();
    } break;
    case 'Z' : {
      RemoveQuaxolUnderCursor();
    } break;
    case 'C' : {
      if (g_vr) {
        g_vr->Recenter();
      }
    } break;
    case 'V' : {
      if (g_vr) {
        g_vr->SetIsUsingVR(!g_vr->IsUsingVR());
      }
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

  g_renderer.m_clearColor = Vec4f(158.0f / 255.0f, 224.0f / 255.0f, 238.0f / 255.0f, 0.0f);
  glClearColor(g_renderer.m_clearColor.x, g_renderer.m_clearColor.y, g_renderer.m_clearColor.z, g_renderer.m_clearColor.w);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  float frameTime = (float)g_renderer.GetFrameTime();
  static bool renderVRUI = true;

  if(VRWrapper::IsUsingVR() && g_vr) {
    Vec2f uiOffset(150.0f, 300.0f);
    g_vr->StartFrame();
    Texture* renderColor;
    Texture* renderDepth;
    glClearColor(g_renderer.m_clearColor.x, g_renderer.m_clearColor.y, g_renderer.m_clearColor.z, g_renderer.m_clearColor.w);
    g_vr->StartLeftEye(&g_camera, &renderColor, &renderDepth);
    g_renderer.RenderAllScenesPerCamera(renderColor, renderDepth);
    if(renderVRUI)
      ImGuiWrapper::Render(frameTime, uiOffset, true /*doUpdate*/);
    glClearColor(g_renderer.m_clearColor.x, g_renderer.m_clearColor.y, g_renderer.m_clearColor.z, g_renderer.m_clearColor.w);
    g_vr->StartRightEye(&g_camera, &renderColor, &renderDepth);
    g_renderer.RenderAllScenesPerCamera(renderColor, renderDepth);
    if(renderVRUI)
      ImGuiWrapper::Render(frameTime, uiOffset, false /*doUpdate*/);
    g_vr->FinishFrame();
  } else {
    Vec2f uiOffset(0, 0);
    g_camera.UpdateRenderMatrix(NULL /*lookOffset*/, NULL /*posOffset*/);
    glClearColor(g_renderer.m_clearColor.x, g_renderer.m_clearColor.y, g_renderer.m_clearColor.z, g_renderer.m_clearColor.w);
    g_renderer.RenderAllScenesPerCamera(NULL /*renderColor*/, NULL /*renderDepth*/);
    ImGuiWrapper::Render(frameTime, uiOffset, true /*doUpdate*/);
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
  
  int guiWidth = 0; // 0 means ImGui will figure it out
  int guiHeight = 0; 
  if(g_vr && g_vr->IsUsingVR()) {
    g_vr->GetPerEyeRenderSize(guiWidth, guiHeight);
  }
  ImGuiWrapper::NewFrame((float)g_renderer.GetFrameTime(), guiWidth, guiHeight);
  
  g_inputHandler.PollJoysticks();
  g_inputHandler.ApplyJoystickInput((float)g_renderer.GetFrameTime());
  ApplyMouseMove();
  
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
}

void glfwErrorCallback(int error, const char* description) {
  printf("GLFW Error: %d :  %s\n", error, description);
}

// Soooo tacky!
#define RUN_TESTS

int main(int argc, char *argv[]) {

  StaticInitialize();

#ifdef RUN_TESTS
  RunTests();
#endif // RUN_TESTS

  // ovr is supposed to preceed glfw
  g_vr = VRWrapper::CreateVR();

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
  if(startFullscreen && g_vr && !g_vr->GetIsDebugDevice()) {
    std::string deviceName = g_vr->GetDeviceName();
    monitor = PlatformWindow::GetRiftMonitorByName(deviceName.c_str());
    vidMode = glfwGetVideoMode(monitor);
    g_vr->GetTotalRenderSize(startWidth, startHeight);
  } else if(startFullscreen) {
    monitor = glfwGetPrimaryMonitor();
    vidMode = glfwGetVideoMode(monitor);
    startWidth = vidMode->width;
    startHeight = vidMode->height;
  } else {
    monitor = glfwGetPrimaryMonitor();
    vidMode = glfwGetVideoMode(monitor);
  }

  glfwWindowHint(GLFW_RED_BITS, vidMode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, vidMode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, vidMode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, vidMode->refreshRate);
  if(!startFullscreen) {
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

  ImGuiWrapper::Init(g_glfwWindow, Key, NULL /*mouseButtonCallback*/);
  //glfwSetKeyCallback(g_glfwWindow, Key);
  
  g_vr->InitializeWindow(g_platformWindow);

  glfwSetCursorPosCallback(g_glfwWindow, PassiveMotion);

  glfwSetWindowRefreshCallback(g_glfwWindow, Draw);

  if(g_vr) {
    g_vr->SetIsUsingVR(startFullscreen && g_vr->GetIsDebugDevice());
  }
  
  if(!Initialize(startWidth, startHeight)) {
    printf("Initialized failed\n");
    return -1;
  }

  ReshapeGL(g_glfwWindow, startWidth, startHeight);

  while(true) {
    StepFrame();
    Draw(g_glfwWindow);
  
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


#else // DERP

void derpglfwFramebufferSizeCallback(
    GLFWwindow* window, int width, int height)
{
  _width = width;
  _height = height;

  glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

  glViewport(0, 0, (GLsizei) (_width), (GLsizei) (_height));

  if(height == 0)
	  height = 1;
	float ratio = 1.0f * width / height;
  gluPerspective(45,ratio,1,1000);
	glMatrixMode(GL_MODELVIEW);
}

void derpKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    Deinitialize();
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

//#define DERP_INLINE_SHADERS
#ifdef DERP_INLINE_SHADERS
GLuint v,f,p;
void loadInlineShaders() {
	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);
  WasGLErrorPlusPrint();

	char vs[] = R"foo(
varying vec3 normal;
uniform vec4 worldPosition;

//in vec3 vertPosition;
//in vec3 vertColor;
//in vec3 vertNormal;
//in vec2 vertTexCoord0;

out vec2 fragTex0;

void main()
{
  gl_Position = ftransform() + worldPosition;	
  normal = gl_Normal; //vertNormal;
  fragTex0 = gl_MultiTexCoord0.xy; // vertTexCoord0;
 
  //normal = gl_Normal;
	//gl_Position = ftransform() + worldPosition;
  //fragTex0 = gl_MultiTexCoord0.xy;
  //fragTex0.xy = gl_Position.xy;
}
)foo";
	char fs[] = R"foo(
varying vec3 normal;

uniform sampler2D texDiffuse0;
in vec2 fragTex0;

void main()
{
	vec4 color;
  color = vec4(0.1,0.2,0.1,1.0); // * 0.1;
  //color.xyz += normal * 0.1; 
  color += texture2D(texDiffuse0, fragTex0);
  color.xy += fragTex0;
  //color.z = 1.0;
  color.a = 1.0;
	gl_FragColor = color;
}
)foo";
  const char* vsPtr = &vs[0]; 
  const char* fsPtr = &fs[0];

	glShaderSource(v, 1, &vsPtr,NULL);
  WasGLErrorPlusPrint();
	glShaderSource(f, 1, &fsPtr,NULL);
  WasGLErrorPlusPrint();

	glCompileShader(v);
  if(!Shader::CheckGLShaderCompileStatus(v, "inline_vertex")) {
    exit(-1);
  }
	glCompileShader(f);
  if(!Shader::CheckGLShaderCompileStatus(f, "inline_frag")) {
    exit(-1);
  }
	
	p = glCreateProgram();
  WasGLErrorPlusPrint();
  glAttachShader(p,f);
  WasGLErrorPlusPrint();
	glAttachShader(p,v);
  WasGLErrorPlusPrint();

	glLinkProgram(p);
  if(!Shader::CheckGLShaderLinkStatus(p, "inline_vert", "inline_frag")) {
    exit(-1);
  }
	glUseProgram(p);
  WasGLErrorPlusPrint();
  glUseProgram(0);
}
#else // DERP_INLINE_SHADERS
fd::Shader derpShader;
#endif // DERP_INLINE_SHADERS

::fd::Texture* g_texture;
void derpwindowRefreshFun(GLFWwindow* window) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  static float timey = 0.0f;
  timey += 0.008f;
  float movey = 1.0f + sinf(timey / 10.0f) * 1.0f;
  char uniformName[] = "worldPosition";
  char texHandle[] = "texDiffuse0";
#ifdef DERP_INLINE_SHADERS
  glUseProgram(p);
  GLint handle = glGetUniformLocation(p, uniformName);
  if (handle == -1) {
    printf ("couldn't get handle %s\n", uniformName);
  } else {
    Vec4f moveIt(movey, movey, movey, 1.0f);
    glUniform4fv(handle, 1, moveIt.raw());
  }

  GLint hTex = glGetUniformLocation(p, texHandle);
  if (hTex == -1) {
    printf ("couldn't get handle %s\n", texHandle);
  } else {
    WasGLErrorPlusPrint();
    glActiveTexture(GL_TEXTURE0);
    WasGLErrorPlusPrint();
    glBindTexture(GL_TEXTURE_2D, g_texture->GetTextureID());
    WasGLErrorPlusPrint();    
    glUniform1i(hTex, 0);
  }

#else //ifdef DERP_INLINE_SHADERS
  derpShader.StartUsing();

  derpShader.getUniform("worldMatrix");
  derpShader.getUniform("worldPosition");
  derpShader.getUniform("cameraPosition");
  derpShader.getUniform("cameraMatrix");
  derpShader.getUniform("projectionMatrix");
  derpShader.getUniform("fourToThree");
  derpShader.getUniform("wPlaneNearFar");

  GLint handle = derpShader.getUniform(uniformName);
  if (handle == -1) {
    printf ("couldn't get handle %s\n", uniformName);
  } else {
    Vec4f moveIt(movey, movey, movey, 1.0f);
    glUniform4fv(handle, 1, moveIt.raw());
  }

  GLint hTex = derpShader.getUniform(texHandle);
  if (hTex == -1) {
    printf ("couldn't get handle %s\n", texHandle);
  } else {
    WasGLErrorPlusPrint();
    glActiveTexture(GL_TEXTURE0);
    WasGLErrorPlusPrint();
    glBindTexture(GL_TEXTURE_2D, g_texture->GetTextureID());
    WasGLErrorPlusPrint();
    glUniform1i(hTex, 0);
  }
#endif //def DERP_INLINE_SHADERS

	glBegin(GL_TRIANGLES);
    glVertex3f(-0.5,-0.5,0.0);
    glColor3f(1.0f, 0.0f, 0.0f);
    glNormal3f(0.7f, 0.7f, 0.7f);
//    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(0.5,0.0,0.0);
    glColor3f(0.0f, 1.0f, 0.0f);
    glNormal3f(0.7f, 0.7f, 0.7f);
//		glTexCoord2f(0.5f, 1.0f);
    glVertex3f(0.0,0.5,0.0);
    glColor3f(0.0f, 0.0f, 1.0f);
    glNormal3f(0.7f, 0.7f, 0.7f);
//		glTexCoord2f(1.0f, 0.5f);
	glEnd();

  glLoadIdentity();
	gluLookAt(0.0,0.0,5.0, 
  		      0.0,0.0,-1.0,
	    		  0.0f,1.0f,0.0f);

	float lpos[4] = {1,0.5,1,0};
  glLightfv(GL_LIGHT0, GL_POSITION, lpos);
  glUseProgram(0);

  glFlush();

  glfwSwapBuffers(window);
}

void glfwErrorCallback(int error, const char* description)
{
    printf("GLFW Error: %s\n", description);
}

int main(int argc, char *argv[]) {
  
  glfwSetErrorCallback(glfwErrorCallback);

  if(!glfwInit()) {
    printf("glfw init fail\n");
    return -1;
  }
  GLFWwindow* window = glfwCreateWindow(640, 480, "fourd", NULL, NULL);
  if(!window) {
    printf("glfwCreateWindow fail\n");
    return -1;
  }
  
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetFramebufferSizeCallback(window, derpglfwFramebufferSizeCallback);
  derpglfwFramebufferSizeCallback(window, 640, 480);
  
  glewExperimental = TRUE;
  if(glewInit() != GLEW_OK) {
    printf("glew init fail\n");
    return -1;
  }

  glfwSetWindowRefreshCallback(window, derpwindowRefreshFun);

  glfwSetKeyCallback(window, derpKey);

  //Initialize();
  
  g_texture = new ::fd::Texture();
  g_texture->LoadFromFile("data\\textures\\orientedTexture.png");

#ifdef DERP_INLINE_SHADERS
  loadInlineShaders();
#else
  derpShader.Release();
  derpShader.LoadFromFile(
      "trivial", "data\\vertTrivial.glsl", "data\\fragTrivial.glsl");
#endif

  while(true) {
    derpwindowRefreshFun(window);
  
    glfwPollEvents();
    if(glfwWindowShouldClose(window)) {
      break;
    }
  }

  glfwTerminate();

  return 0;
}

#endif // DERP
