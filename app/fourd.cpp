#include <math.h>
#include <memory>

#include <GL/glew.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#include <GL/freeglut.h>
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
#include "../common/raycast_shape.h"
#include "../common/components/animated_rotation.h"
#include "../common/components/periodic_motion.h"
#include "../common/components/physics_component.h"
#include "../common/components/timed_death.h"
#include "entity.h"
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

// Setup a render target approach
// Get multi-view working
// Get a post effect working
// Integrate rift

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
::fd::PlatformWindow* g_platformWindow;

::fd::VRWrapper* g_vr = NULL;
Mat4f g_debugHeadPose;

bool LoadShader(const char* shaderName) {
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
      return false;
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
    return false;
  }

  shaderMem.release();
  g_shader = pShader;
  g_scene.m_pQuaxolShader = g_shader;

  g_currentShader = vertPrefix + baseNameWithExt;
  printf("Loaded shader %s\n", vertName.c_str());

  return true;
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

bool Initialize() {
  //tesseract.buildQuad(10.0f, Vec4f(-20.0, 0, -20.0, 0));
  //tesseract.buildCube(10.0f, Vec4f(0, 0, 0, 0));
  //tesseract.buildTesseract(10.0f, Vec4f(-5.1f,-5.1f,-5.1f,-5.1f), Vec4f(0,0,0,0));
  tesseract.buildTesseract(g_blockSize, Vec4f(0,0,0,0.0f), Vec4f(0,0,0,0));
 
  // Set up some reasonable defaults
  g_camera.SetZProjection(_width, _height, 90.0f /* fov */,
      0.1f /* zNear */, 10000.0f /* zFar */);
  g_camera.SetWProjection(
      0.0f /* wNear */, 40.0f /* wFar */, 0.5f /* wScreenRatio */);
  g_camera.setMovementMode(Camera::MovementMode::LOOK); //ORBIT); //LOOK);
  g_camera.SetCameraPosition(Vec4f(0.5f, 0.5f, 15.5f, 4.5f));
  //g_camera.SetCameraPosition(Vec4f(100.5f, 100.5f, 115.5f, 100.5f));
  g_camera.ApplyRotationInput(-(float)PI / 2.0f, Camera::FORWARD, Camera::UP);
  g_debugHeadPose.storeIdentity();

  //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearColor(158.0f / 255.0f, 224.0f / 155.0f, 238.0f / 255.0f, 0.0f);
  WasGLErrorPlusPrint();
  glClearDepth(1.0f);
  WasGLErrorPlusPrint();
  glDisable(GL_CULL_FACE); // no backface culling for 4d
  WasGLErrorPlusPrint();
  glShadeModel(GL_FLAT);
  WasGLErrorPlusPrint();
  //glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  WasGLErrorPlusPrint();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //GL_FILL); //GL_LINE);
  WasGLErrorPlusPrint();
  //glDisable(GL_MIPMAP);
  //WasGLErrorPlusPrint();

  g_renderer.ToggleAlphaDepthModes(Render::AlphaOnDepthOffSrcDest);
  // Just preload the shaders to check for compile errors
  // Last one will be "current"
  if (!LoadShader("AlphaTest")
    || !LoadShader("AlphaTestTex")
    || !LoadShader("ColorBlend")
    || !LoadShader("BlendNoTex")
    || !LoadShader("ColorBlendClipped")
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
  g_scene.Initialize();

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

  return true;
}

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

void Deinitialize(void) {
  ::fd::Texture::DeinitializeTextureCache();
  ::fd::Shader::ClearShaderHash();
  ::fd::Platform::Shutdown();
}

void ReshapeGL(int width, int height) {
  _width = width;
  _height = height;
  glViewport(0, 0, (GLsizei) (_width), (GLsizei) (_height));
  UpdatePerspective();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glutPostRedisplay();
}

int mouseX = 0;
int mouseY = 0;
int accumulatedMouseX = 0;
int accumulatedMouseY = 0;

void PassiveMotion(int x, int y) {
  int threshold = 20;
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
      glutWarpPointer(newX, newY);
    }
  }
}

void ApplyMouseMove() {
  static float moveAmount = 0.01f;
  if (accumulatedMouseX) {
    g_camera.ApplyRotationInput(moveAmount * -accumulatedMouseX, Camera::FORWARD, Camera::RIGHT);
    accumulatedMouseX = 0;
  }

  if (accumulatedMouseY) {
    g_camera.ApplyRotationInput(moveAmount * accumulatedMouseY, Camera::FORWARD, Camera::UP);
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
  Vec4f ray = g_camera.getCameraForward();
  ray *= 1000.0f;

  QuaxolSpec gridPos;
  if(g_scene.m_pPhysics->RayCastToOpenQuaxol(
      position, ray, &gridPos, NULL /*hitPos*/)) {
    g_scene.SetQuaxolAt(gridPos, true /*present*/);
  }
}

void RemoveQuaxolUnderCursor() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getCameraForward();
  ray *= 1000.0f;

  QuaxolSpec gridPos;
  if(g_scene.m_pPhysics->RayCastToPresentQuaxol(
      position, ray, &gridPos, NULL /*hitPos*/)) {
    g_scene.SetQuaxolAt(gridPos, false /*present*/);
  }
}

void AddRaycastEntity() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getCameraForward();
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
  Vec4f ray = g_camera.getCameraForward();
  ray *= 10.0f;
  DelegateN<void, int, int, int, int, const Vec4f&, const Vec4f&> delegate;
  delegate.Bind(AddTesseractLineCallback);
  g_scene.m_pPhysics->LineDraw4D(cameraPos, ray, delegate);
  
  QuaxolSpec offset(0,0,0,0);
  g_scene.m_pQuaxolChunk->LoadFromList(&(g_scene.m_quaxols), &offset);  
}

void Update(int key, int x, int y) {
  UNUSED(x); UNUSED(y); // Required by glut prototype.
  static float moveAmount = 1.0f;
  static float rollAmount = moveAmount * 2 * (float)PI / 100.0f;


  bool isShift = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;

  switch (key) {
    case '`' : {
      ToggleMouseCapture();
    } break;
    case '!' : {
      g_renderer.ToggleAlphaDepthModes(Render::EToggleModes);
    } break;
    case '@' : {
      LoadShader("BlendNoTex");
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
      LoadShader("ColorBlend");
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
    case '+' : {
      static bool fill = false;
      fill = !fill;
      if (fill) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      }
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
      Vec4f jumpAmount = -(g_scene.m_pPhysics->m_gravity) * 2.0f;
      g_camera.GetComponentBus().SendSignal("AddImpulse", SignalN<Vec4f>(), jumpAmount);
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
        //g_camera.setMovementMode(Camera::ORBIT);
        g_camera.setMovementMode(Camera::WALK);
        g_camera.GetComponentBus().SendSignal(std::string("DestroyPhysics"), SignalN<>());
        RaycastShape* shape = new RaycastShape(g_scene.m_pPhysics);
        shape->AddCapsuleRays(g_blockSize);
        PhysicsComponent* physicsComp =
            new PhysicsComponent(g_scene.m_pPhysics, shape);
        g_camera.GetComponentBus().AddComponent(physicsComp);
      } else {
        g_camera.setMovementMode(Camera::LOOK);
        g_camera.GetComponentBus().SendSignal(std::string("DestroyPhysics"), SignalN<>());
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
  glutPostRedisplay();
}

void Draw(void) {
  if (!g_shader)
    return;
    
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  if(VRWrapper::IsUsingVR() && g_vr) {
    g_vr->StartFrame();
    g_vr->StartLeftEye(&g_camera);
    g_renderer.RenderAllScenesPerCamera();
    g_vr->StartRightEye(&g_camera);
    g_renderer.RenderAllScenesPerCamera();
    g_vr->FinishFrame();
  } else {
    g_camera.NoOffsetUpdate();
    g_renderer.RenderAllScenesPerCamera();
  }

  glFlush();
  glutSwapBuffers();
}

void Key(unsigned char key, int x, int y) {
  Update(key, x, y);
  return;
}

void MouseClick(int type, int up, int x, int y) {
  UNUSED(x); UNUSED(y); UNUSED(type); UNUSED(up);
  //printf("Click type:%d up:%d x:%d y:%d\n", type, up, x, y);
}

void Motion(int x, int y) {
  UNUSED(x); UNUSED(y);
  //printf("Motion x:%d y:%d\n", x, y);
}

void RaycastToOpenQuaxol() {
  Vec4f position = g_camera.getCameraPos();
  Vec4f ray = g_camera.getCameraForward();
  ray *= 1000.0f;

  Vec4f hitPos(0.0f, 0.0f, -1000.0f, 0.0f);
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
  Vec4f ray = g_camera.getCameraForward();
  ray *= 1000.0f;
  float dist;

  Vec4f hitPos(0.0f, 0.0f, -1000.0f, 0.0f);

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
  if(g_camera.getMovementMode() == Camera::LOOK) {
    static bool quaxolMode = true;
    if(quaxolMode) {
      RaycastToOpenQuaxol();
    } else {
      RaycastToCollsion();
    }
  }
}

void OnIdle() {
  ApplyMouseMove();
  g_renderer.Step();
  g_scene.Step((float)g_renderer.GetFrameTime());

  // TODO: do some damn font code
  //static int framecount = 0;
  //if(framecount++ > 200) {
  //  printf("frametime:%f\n", g_renderer.GetFrameTime());
  //  framecount = 0;
  //}

  UpdatePointerEntity();

  glutPostRedisplay();
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

  Shader::TestShaderHash();
  Camera::TestComponents();
  Physics::TestPhysics();
}

//#define DERP
#ifndef DERP

// Soooo tacky!
#define RUN_TESTS

  int main(int argc, char *argv[]) {

  StaticInitialize();

#ifdef RUN_TESTS
  RunTests();
#endif // RUN_TESTS

  glutInit(&argc, argv);
  glutInitWindowPosition(0, 0);
  int startWidth = 640;
  int startHeight = 580;
  glutInitWindowSize(startWidth, startHeight);
  glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE);
  //glutInitContextVersion(4, 3);
  GLint contextFlags = GLUT_CORE_PROFILE; // GLUT_FORWARD_COMPATIBLE;
//#ifdef _DEBUG
//  contextFlags |= GLUT_DEBUG;
//#endif // _DEBUG
  glutInitContextFlags(contextFlags);
  GLint contextProfile = 0; // GLUT_CORE_PROFILE;
  glutInitContextProfile(contextProfile);
    
  glutCreateWindow(argv[0]);
  
  char windowTitle[] = "fourd";
  g_platformWindow = ::fd::Platform::Init(windowTitle, startWidth, startHeight);
  
  glewExperimental=TRUE;
  GLenum err;
  if((err = glewInit()) != GLEW_OK) {
    printf("Glew init fail: Error: %s\n", glewGetErrorString(err));
    return false;
  }
  
  g_vr = VRWrapper::CreateVR(g_platformWindow);

  glutReshapeFunc(ReshapeGL);
  glutKeyboardFunc(Key);
  glutMouseFunc(MouseClick);
  glutPassiveMotionFunc(PassiveMotion);
  glutMotionFunc(Motion);
  //glutSpecialFunc(Update);
  glutDisplayFunc(Draw);
  glutIdleFunc(OnIdle);
  Initialize();
  glutMainLoop();
  return 0;
}


#else // DERP

void derpIdle() {
  glutPostRedisplay();
}

void derpReshapeGL(int width, int height) {
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

  glutPostRedisplay();
}

void derpKey(unsigned char key, int x, int y) {
  switch(key) {
      case 27: {
      Deinitialize();
      exit(0);
    } break;
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
  //color += texture2D(texDiffuse0, fragTex0);
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

::fd::Texture g_texture;
void derpRenderScene(void) {

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
    glBindTexture(GL_TEXTURE_2D, g_texture.GetTextureID());
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
    glBindTexture(GL_TEXTURE_2D, g_texture.GetTextureID());
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

//#ifdef DERP_INLINE_SHADERS
//  glUseProgram(p);
//	glutSolidTeapot(1);
//  glUseProgram(0);
//#else
//  derpShader.StartUsing();
//	glutSolidTeapot(1);
//  derpShader.StopUsing();
//#endif //def DERP_INLINE_SHADERS

  glFlush();

	glutSwapBuffers();
}

int main(int argc, char *argv[]) {
  
  glutInit(&argc, argv);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(640, 480);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
//  glutInitContextVersion(3, 2);
  GLint contextFlags = GLUT_CORE_PROFILE; // GLUT_FORWARD_COMPATIBLE;
//#ifdef _DEBUG
//  contextFlags |= GLUT_DEBUG;
//#endif // _DEBUG
  glutInitContextFlags(contextFlags);
    
  glutCreateWindow(argv[0]);
  
  glewExperimental = TRUE;
  if(glewInit() != GLEW_OK) {
    printf("glew init fail\n");
    return false;
  }

  glutReshapeFunc(derpReshapeGL);
  glutKeyboardFunc(derpKey);
  glutDisplayFunc(derpRenderScene);
  glutIdleFunc(derpIdle);

  Initialize();
  
  g_texture.LoadFromFile("data\\textures\\orientedTexture.png");

  WasGLErrorPlusPrint();

#ifdef DERP_INLINE_SHADERS
  loadInlineShaders();
#else
  derpShader.Release();
  derpShader.LoadFromFile(
      "trivial", "data\\vertTrivial.glsl", "data\\fragTrivial.glsl");
#endif
  g_texture.LoadFromFile("data\\orientedTexture.png");


  glutMainLoop();
  return 0;
}

#endif // DERP
