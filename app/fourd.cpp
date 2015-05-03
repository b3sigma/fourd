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
#include "../common/components/animated_rotation.h"
#include "../common/components/periodic_motion.h"
#include "../common/components/timed_death.h"
#include "entity.h"
#include "render.h"
#include "scene.h"
#include "shader.h"
#include "texture.h"
#include "glhelper.h"

using namespace ::fd;

// Setup a render target approach
// Get multi-view working
// Get a post effect working
// Integrate rift

int _width = 800;
int _height = 600;
Mesh tesseract;
::fd::Scene g_scene;
::fd::Camera g_camera;
::fd::Render g_renderer;
::fd::Texture g_texture;
::fd::Shader* g_shader = NULL;
bool g_captureMouse = false;
HWND g_windowHandle;

bool LoadShader(const char* shaderName) {
  std::string shaderDir = "data\\";
  std::string vertName = shaderDir + std::string("vert")
    + std::string(shaderName) + std::string(".glsl");
  std::string fragName = shaderDir + std::string("frag")
    + std::string(shaderName) + std::string(".glsl");
  std::string commonVert = shaderDir + std::string("vertCommonTransform.glsl");

  std::unique_ptr<::fd::Shader> shaderMem;
  ::fd::Shader* pShader = ::fd::Shader::GetShaderByRefName(shaderName);
  if (pShader) {
    pShader->Release();
  } else {
    pShader = new ::fd::Shader();
    shaderMem.reset(pShader);
  }

  pShader->AddSubShader(commonVert.c_str(), GL_VERTEX_SHADER); 
  if(!pShader->LoadFromFile(shaderName, vertName.c_str(), fragName.c_str())) {
    printf("Failed loading shader!\n");
    return false;
  }

  shaderMem.release();
  g_shader = pShader;
  g_scene.m_pQuaxolShader = g_shader;

  return true;
}

bool LoadLevel(const char* levelName) {
  std::string levelPath = "data\\";
  std::string nameBase(levelName);
  std::string nameExt = ".txt"; // heh, I guess ext based format? hate you
  std::string fullName = levelPath + nameBase + nameExt;
  ChunkLoader chunks;
  if (chunks.LoadFromFile(fullName.c_str())) {
    std::swap(g_scene.m_quaxols, chunks.quaxols_);
    printf("Level (%s) loaded %d quaxols!\n",
        fullName.c_str(), (int)g_scene.m_quaxols.size());
    return true;
  }
  else {
    printf("Couldn't load the level!\n");
    return false;
  }
}

void SetAlphaAndDisableDepth(bool bAlphaAndDisableDepth) {
  if (bAlphaAndDisableDepth) {
    glEnable(GL_BLEND);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    glDisable(GL_ALPHA_TEST);

    glDepthFunc(GL_ALWAYS);
    glDisable(GL_DEPTH_TEST);
  } else {
    
    //glDisable(GL_BLEND);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 154.0f / 255.0f);
    
    glDepthFunc(GL_LESS); // GL_GREATER); //GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
  }
}

bool Initialize() {
  //tesseract.buildQuad(10.0f, Vec4f(-20.0, 0, -20.0, 0));
  //tesseract.buildCube(10.0f, Vec4f(0, 0, 0, 0));
  //tesseract.buildTesseract(10.0f, Vec4f(0,0,0,0.0f), Vec4f(0,0,0,0));
  tesseract.buildTesseract(10.0f, Vec4f(-5.1f,-5.1f,-5.1f,-5.1f), Vec4f(0,0,0,0));
  
  // Set up some reasonable defaults
  g_camera.SetZProjection(_width, _height, 90.0f /* fov */,
      0.1f /* zNear */, 10000.0f /* zFar */);
  g_camera.SetWProjection(
      0.0f /* wNear */, 40.0f /* wFar */, 0.5f /* wScreenRatio */);
  g_camera.setMovementMode(Camera::MovementMode::LOOK); //ORBIT); //LOOK);
  g_camera.SetCameraPosition(Vec4f(100.5f, 100.5f, 115.5f, 100.5f));
  g_camera.ApplyRotationInput(-(float)PI / 2.0f, Camera::FORWARD, Camera::UP);

  
  g_texture.LoadFromFile("data\\orientedTexture.png");
  WasGLErrorPlusPrint();

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  WasGLErrorPlusPrint();
  glClearDepth(1.0f);
  WasGLErrorPlusPrint();
  glDisable(GL_CULL_FACE); // no backface culling for 4d
  WasGLErrorPlusPrint();
  //glBlendEquation(GL_ADD);
  WasGLErrorPlusPrint();
  //glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA); //GL_ONE_MINUS_SRC_ALPHA); // 
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 
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

  SetAlphaAndDisableDepth(true);
  // Just preload the shaders to check for compile errors
  // Last one will be "current"
  if (!LoadShader("AlphaTest") || !LoadShader("BlendNoTex")
    || !LoadShader("AlphaTestTex")) {
    printf("Shader loading failed\n");
    exit(-1);
  }
  
  LoadLevel("level_4d_base_offset");
  g_scene.AddCamera(&g_camera);
  g_scene.m_pQuaxolMesh = &tesseract;
  g_scene.m_pQuaxolShader = g_shader;
  g_scene.m_pQuaxolTex = &g_texture;

  WasGLErrorPlusPrint();

  return true;
}

void UpdatePerspective() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  g_camera.SetZProjection(_width, _height,
      g_camera._zFov, g_camera._zNear, g_camera._zFar);
}

void SetSimpleProjectiveMode() {
  g_camera.SetWProjection(-5.5f, 5.5f, 0.9f);
  LoadShader("AlphaTest");
  SetAlphaAndDisableDepth(false);
  UpdatePerspective();
}

void Deinitialize(void) {
  ::fd::Shader::ClearShaderHash();
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
    const int edgeWarp = 30;
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

void ToggleMouseCapture() {
  g_captureMouse = !g_captureMouse;
  if (g_captureMouse) {
    RECT windowRect; // will include border
    GetWindowRect(g_windowHandle, &windowRect);
    RECT clientRect; // local window space
    GetClientRect(g_windowHandle, &clientRect);

    RECT interiorRect = clientRect;
    interiorRect.left += windowRect.left;
    interiorRect.right += windowRect.left;
    interiorRect.top += windowRect.top;
    interiorRect.bottom += windowRect.top;

    // This is actually wrong, we aren't handling the border correctly,
    // but it doesn't matter as the mouse move wrap handles it.
    BOOL result = ClipCursor(&interiorRect);

    glutSetCursor(GLUT_CURSOR_NONE);
  } else {
    ClipCursor(NULL);
    glutSetCursor(GLUT_CURSOR_INHERIT);
  }
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
      SetAlphaAndDisableDepth(glIsEnabled(GL_DEPTH_TEST) == GL_TRUE);
    } break;
    case '@' : {
      LoadShader("BlendNoTex");
      UpdatePerspective();
    } break;
    case '#' : {
      LoadShader("AlphaTest");
      UpdatePerspective();
    } break;
    case '$' : {
      LoadShader("AlphaTestTex");
      UpdatePerspective();
    } break;
    case '%' : {
      SetSimpleProjectiveMode();
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
      tesseract.buildReferenceTesseract(10.0f, Vec4f(0.5, 0.5, 0.5, 0.5), Vec4f(0, 0, 0, 0));
    } break;
    case '6' : {
      tesseract.buildCircle(10.0f, Vec4f(10.5, 0.5, 0.5, 0.5), Vec4f(1, 0, 0, 0), Vec4f(0, 1, 0, 0), 6);
    } break;
    case '7' : {
      tesseract.buildCylinder(10.0f, 10.f, 60);
    } break;
    case '8' : {
      tesseract.buildFourCylinder(10.0f, 10.f, 10.0f, 6);
    } break;
    case '(' : {
      static bool fill = false;
      fill = !fill;
      if (fill) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      }
    } break;
    case '9' : {
      LoadLevel("level_4d_base_offset");
    } break;
    case '0' : {
      LoadLevel("level_trivial");
    } break;
    case 27: {
      Deinitialize();
      exit(0);
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
          g_camera._wNear - 1.0f, g_camera._wFar, g_camera._wScreenSizeRatio);
    } break;
    case 'c' : {
      g_camera.SetWProjection(
          g_camera._wNear + 1.0f, g_camera._wFar, g_camera._wScreenSizeRatio);
    } break;
    case 'v' : {
      g_camera.SetWProjection(
          g_camera._wNear, g_camera._wFar - 1.0f, g_camera._wScreenSizeRatio);
    } break;
    case 'b' : {
      g_camera.SetWProjection(
          g_camera._wNear, g_camera._wFar + 1.0f, g_camera._wScreenSizeRatio);
    } break;
    case 'n' : {
      g_camera.SetWProjection(
          g_camera._wNear, g_camera._wFar, g_camera._wScreenSizeRatio - 0.1f);
    } break;
    case 'm' : {
      g_camera.SetWProjection(
          g_camera._wNear, g_camera._wFar, g_camera._wScreenSizeRatio + 0.1f);
    } break;
    case '?' : {
      g_camera.printIt();
    } break;
    case '\'' : {
      tesseract.printIt();
    } break;
    case '[' : {
      if (g_camera.getMovementMode() == Camera::LOOK) {
        g_camera.setMovementMode(Camera::ORBIT);
      } else {
        g_camera.setMovementMode(Camera::LOOK);
      }
    } break;
    case ']' : { // ortho projection
      static float savedWScreenRatio = 0.5f;
      float newWScreenRatio = savedWScreenRatio;
      if (g_camera._wScreenSizeRatio == 1.0f) {
        g_camera.SetWProjection(
            g_camera._wNear, g_camera._wFar, savedWScreenRatio);
      } else {
        savedWScreenRatio = g_camera._wScreenSizeRatio;
        g_camera.SetWProjection(
            g_camera._wNear, g_camera._wFar, 1.0f);
      }
    } break;
    case 'z' : {
      Entity* pEntity = g_scene.AddEntity();
      // ugh need like a mesh manager and better approach to shader handling
      pEntity->Initialize(&tesseract, g_shader, NULL);
      pEntity->m_orientation.storeIdentity();
      
      Vec4f forwardDir = g_camera.getCameraMatrix()[Camera::FORWARD];
      forwardDir *= -50.0f;
      pEntity->m_position = g_camera._cameraPos + forwardDir;
      
      pEntity->GetComponentBus().AddComponent(
          new AnimatedRotation((float)PI * 10.0f, Camera::RIGHT, Camera::INSIDE,
          20.0f, true));
      pEntity->GetComponentBus().AddComponent(
          new TimedDeath(21.0f /* duration */));
    } break;
    case 'Z' : {
      Entity* pEntity = g_scene.AddEntity();
      // ugh need like a mesh manager and better approach to shader handling
      pEntity->Initialize(&tesseract, g_shader, NULL);
      pEntity->m_orientation.storeIdentity();

      // Also put it in front of the camera.
      Vec4f forwardDir = g_camera.getCameraMatrix()[Camera::FORWARD];
      forwardDir *= -50.0f;
      pEntity->m_position = g_camera._cameraPos + forwardDir;

      Vec4f tangentDir = g_camera.getCameraMatrix()[Camera::INSIDE];
      tangentDir *= -50.0f;

      pEntity->GetComponentBus().AddComponent(
          new PeriodicMotion(10.0f /* duration */, 2.0f /* period */,
              0.0f /* phase */, tangentDir));
      pEntity->GetComponentBus().AddComponent(
          new TimedDeath(13.0f /* duration */));
    } break;
  }
  glutPostRedisplay();
}

void Draw(void) {
  if (!g_shader)
    return;
    
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  // fix the rotation to be smoother
  // figure out the clipping issues (negative w?)
  // normalize the input amounts
  // refactor the input system
  // switch the mat4 class to be column major?
  // multi-view rendering

  g_scene.RenderEntitiesStupidly();

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

void OnIdle() {
  ApplyMouseMove();
  g_renderer.Step();
  g_scene.Step((float)g_renderer.GetFrameTime());
  
  glutPostRedisplay();
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
  rotXFourth.buildRotation((float)PI / 2.0, 1, 0);
  Mat4f rotXEighth;
  rotXEighth.buildRotation((float)PI / 4.0, 1, 0);
  assert(iden == (rotXFourth * rotXFourth * rotXFourth * rotXFourth * iden));
  assert(rotXEighth * rotXEighth == rotXFourth);
  assert(!(rotXFourth * rotXFourth == rotXFourth));

  //Mat4f threeMat;
  //threeMat.eigen().

  Shader::TestShaderHash();
  Camera::TestComponents();
}

//#define DERP
#ifndef DERP

// Soooo tacky!
#define RUN_TESTS

int main(int argc, char *argv[]) {
#ifdef RUN_TESTS
  RunTests();
#endif // RUN_TESTS
  
  glutInit(&argc, argv);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(640, 580);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  //glutInitContextVersion(3, 2);
  GLint contextFlags = GLUT_CORE_PROFILE; // GLUT_FORWARD_COMPATIBLE;
//#ifdef _DEBUG
//  contextFlags |= GLUT_DEBUG;
//#endif // _DEBUG
  glutInitContextFlags(contextFlags);
    
  glutCreateWindow(argv[0]);
  
  char windowTitle[] = "fourd";
  glutSetWindowTitle(windowTitle);
  g_windowHandle = FindWindow(NULL, windowTitle);

  glewExperimental=TRUE;
  GLenum err;
  if((err = glewInit()) != GLEW_OK) {
    printf("Glew init fail: Error: %s\n", glewGetErrorString(err));
    return false;
  }

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
