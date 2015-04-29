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
#include "render.h"
#include "../common/chunkloader.h"
#include "shader.h"

#include "texture.h"

using namespace ::fd;

// apparantly the right way to do most of these in GL is to make a
// shared uniform buffer
// look like 56 floats would apply so far
// doing things so inefficiently already that much more refactoring necessary
GLint hPosition;
GLint hColor;
GLint hWorldMatrix;
GLint hWorldPosition;
GLint hCameraPosition;
GLint hCameraMatrix;
GLint hProjectionMatrix;
GLint hFourToThree;
GLint hWPlaneNearFar;
GLint hProjectionFlags;

// TODO: Move most of the GL code to render, including the camera
// Setup a render target approach
// Get multi-view working
// Get a post effect working
// Integrate rift

Mat4f worldMatrix;
Mat4f projectionMatrix;
Mat4f fourToThree;
Vec4f wPlaneNearFar;
Vec4f wProjectionFlags;
Mesh tesseract;
Camera _camera;
float _fov = 90.0f;
float _near = 0.1f;
float _far = 100000.0f;
int _width = 800;
int _height = 600;
int cubeIndex = 0;
::fd::Render renderer;
::fd::TVecQuaxol g_quaxols;
::fd::Texture g_texture;
::fd::Shader* g_shader = NULL;

// trying out different naming conventions ok? quit complaining
// couple days later: suuuure, "trying out" and not "whatever the fuck today"
// don't worry though, "I'll clean it up later" bwahahahaha

typedef std::vector<Vec4f> VectorList;
VectorList colorArray;
void buildColorArray() {
  int numSteps = 8;
  colorArray.reserve(numSteps);
  for (int steps = 0; steps < numSteps; steps++) {
    colorArray.push_back(Vec4f(0, (float)(steps + 1) / (float)numSteps, 0, 1));
  }
}

void SetCommonShaderHandles(::fd::Shader* pShader) {
  pShader->StartUsing();
  hPosition = pShader->getAttrib("vertPosition");
  hColor = pShader->getAttrib("vertColor");
  hWorldMatrix = pShader->getUniform("worldMatrix");
  hWorldPosition = pShader->getUniform("worldPosition");
  hCameraPosition = pShader->getUniform("cameraPosition");
  hCameraMatrix = pShader->getUniform("cameraMatrix");
  hProjectionMatrix = pShader->getUniform("projectionMatrix");
  hFourToThree = pShader->getUniform("fourToThree");
  hWPlaneNearFar = pShader->getUniform("wPlaneNearFar");
  hProjectionFlags = pShader->getUniform("wProjectionFlags");
  pShader->StopUsing();
}

bool LoadShader(const char* shaderName) {
  std::string shaderDir = "data\\";
  std::string vertName = shaderDir + std::string("vert")
    + std::string(shaderName) + std::string(".glsl");
  std::string fragName = shaderDir + std::string("frag")
    + std::string(shaderName) + std::string(".glsl");

  ::fd::Shader* pExisting = ::fd::Shader::GetShaderByRefName(shaderName);
  if (pExisting) {
    //// Shader reloading is broken?
    //g_shader = pExisting;
    //SetCommonShaderHandles(pExisting);
    //return true;
    delete pExisting;
  }

  std::unique_ptr<::fd::Shader> pShader(new ::fd::Shader());

  if(!pShader->LoadFromFile(shaderName, vertName.c_str(), fragName.c_str())) {
    printf("Failed loading shader!\n");
    return false;
  }

  SetCommonShaderHandles(pShader.get());

  g_shader = pShader.release();
  
  return true;
}

bool LoadLevel(const char* levelName) {
  std::string levelPath = "data\\";
  std::string nameBase(levelName);
  std::string nameExt = ".txt"; // heh, I guess ext based format? hate you
  std::string fullName = levelPath + nameBase + nameExt;
  ChunkLoader chunks;
  if (chunks.LoadFromFile(fullName.c_str())) {
    std::swap(g_quaxols, chunks.quaxols_);
    printf("Level (%s) loaded %d quaxols!\n",
        fullName.c_str(), (int)g_quaxols.size());
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
  wProjectionFlags.x = 1.0f; // use projective 4d mode
  wProjectionFlags.y = 0.0f; // project in instead of out
  wProjectionFlags.z = 1.0f; // ratio projection enabled
  
  // Set up some reasonable defaults
  _camera.setMovementMode(Camera::MovementMode::LOOK); //ORBIT); //LOOK);
  _camera.SetCameraPosition(Vec4f(100.5f, 100.5f, 115.5f, 100.5f));
  _camera.ApplyRotationInput(-(float)PI / 2.0f, Camera::FORWARD, Camera::UP);
  
  LoadLevel("level_4d_base_offset");
  
  g_texture.LoadFromFile("data\\orientedTexture.png");

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0f);

  glDisable(GL_CULL_FACE); // no backface culling for 4d
  glBlendEquation(GL_ADD);
  //glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA); //GL_ONE_MINUS_SRC_ALPHA); // 
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 
  glShadeModel(GL_FLAT);
  //glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //GL_FILL); //GL_LINE);

  SetAlphaAndDisableDepth(false);
  // Just preload the shaders to check for compile errors
  // Last one will be "current"
  if (!LoadShader("BlendNoTex") || !LoadShader("AlphaTest")) {
    printf("Shader loading failed\n");
    exit(-1);
  }

  worldMatrix.storeIdentity();
  projectionMatrix.storeIdentity();
  fourToThree.storeIdentity();
  float nearInside = 1.0f;
  //float farInside = 1000.0f;
  fourToThree[0][0] = nearInside;
  fourToThree[1][1] = nearInside;
  fourToThree[2][2] = nearInside;
  wPlaneNearFar.x = 1.0f;
  wPlaneNearFar.y = 40.0f;
  wPlaneNearFar.z = 0.5f; // size of far w plane / size of near w plane

  buildColorArray();

  return true;
}

void Deinitialize(void) {
  ::fd::Shader::ClearShaderHash();
}

void UpdatePerspective() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float aspect = static_cast<float>(_width) / static_cast<float>(_height);
  projectionMatrix.build3dProjection(_fov, aspect, _near, _far);

  if(!g_shader)
    return;
  g_shader->StartUsing();
  glUniformMatrix4fv(hProjectionMatrix, 1, GL_FALSE, projectionMatrix.raw());
  g_shader->StopUsing();
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

void Update(int key, int x, int y) {
  UNUSED(x); UNUSED(y); // Required by glut prototype.
  static float moveAmount = 1.0f;
  static float rollAmount = moveAmount * 2 * (float)PI / 100.0f;

  bool isShift = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;

  switch (key) {
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
      LoadLevel("level_arch_wth_w_overhang");
    } break;
    case 27: {
      Deinitialize();
      exit(0);
    } break;
    case ',' : {
      int numCubes = max((tesseract.getNumberTriangles() / 12), 1);
      cubeIndex = (cubeIndex + 1) % numCubes;
     } break;
    case '.' : {
      int numCubes = max((tesseract.getNumberTriangles() / 12), 1);
      cubeIndex = (cubeIndex - 1);
      if (cubeIndex < 0) {
        cubeIndex = numCubes - 1;
      }
    } break;
    case 'a' : {
      _camera.ApplyTranslationInput(-moveAmount, Camera::RIGHT);
    } break;
    case 'd' : {
      _camera.ApplyTranslationInput(moveAmount, Camera::RIGHT);
    } break;
    case 'w' : {
      _camera.ApplyTranslationInput(-moveAmount, Camera::FORWARD);
    } break;
    case 's' : {
      _camera.ApplyTranslationInput(moveAmount, Camera::FORWARD);
    } break;
    case 'q' : {
      _camera.ApplyTranslationInput(-moveAmount, Camera::UP);
    } break;
    case 'e' : {
      _camera.ApplyTranslationInput(moveAmount, Camera::UP);
    } break;
    case 'r' : {
      _camera.ApplyTranslationInput(moveAmount, Camera::INSIDE);
    } break;
    case 'f' : {
      _camera.ApplyTranslationInput(-moveAmount, Camera::INSIDE);
    } break;
    case 't' : {
      _camera.ApplyRollInput(-rollAmount, Camera::RIGHT, Camera::UP);
    } break;
    case 'g' : {
      _camera.ApplyRollInput(rollAmount, Camera::RIGHT, Camera::UP);
    } break;
    case 'y' : {
      _camera.ApplyRollInput(-rollAmount, Camera::INSIDE, Camera::RIGHT);
    } break;
    case 'h' : {
      _camera.ApplyRollInput(rollAmount, Camera::INSIDE, Camera::RIGHT);
    } break;
    case 'u' : {
      _camera.ApplyRollInput(-rollAmount, Camera::UP, Camera::INSIDE);
    } break;
    case 'j' : {
      _camera.ApplyRollInput(rollAmount, Camera::UP, Camera::INSIDE);
    } break;
    case 'i' : {
      _camera.ApplyWorldRotation(0.5f * (float)PI, Camera::INSIDE, Camera::RIGHT);
    } break;
    case 'k' : {
      _camera.ApplyWorldRotation(-0.5f * (float)PI, Camera::INSIDE, Camera::RIGHT);
    } break;
    // Sure this looks like an unsorted mess, but is spatially aligned kinda.
    case 'x' : {
      wPlaneNearFar.x -= 1.0f;
      //_near *= 0.1f;
      //UpdatePerspective();
    } break;
    case 'c' : {
      wPlaneNearFar.x += 1.0f;
      //_near *= 10.0f;
      //UpdatePerspective();
    } break;
    case 'v' : {
      wPlaneNearFar.y -= 1.0f;
      //_far *= 0.1f;
      //UpdatePerspective();
    } break;
    case 'b' : {
      wPlaneNearFar.y += 1.0f;
      //_far *= 10.0f;
      //UpdatePerspective();
    } break;
    case 'n' : {
      wPlaneNearFar.z -= 0.1f;
      //_fov -= 5.0f;
      //UpdatePerspective();
    } break;
    case 'm' : {
      wPlaneNearFar.z += 0.1f;
      //_fov += 5.0f;
      //UpdatePerspective();
    } break;
    case '?' : {
      _camera.printIt();
      printf("\nwPlaneNearFar\n");
      wPlaneNearFar.printIt();
      printf("\nwProjectionFlags\n");
      wProjectionFlags.printIt();
      printf("\n");
    } break;
    case '\'' : {
      tesseract.printIt();
    } break;
    case '[' : {
      if (_camera.getMovementMode() == Camera::LOOK) {
        _camera.setMovementMode(Camera::ORBIT);
      } else {
        _camera.setMovementMode(Camera::LOOK);
      }
    } break;
    case ']' : {
      if (wProjectionFlags.x == 0.0f) {
        wProjectionFlags.x = 1.0f;
      } else {
        wProjectionFlags.x = 0.0f;
      }
    } break;
    case 'o' : {
      if (wProjectionFlags.y == 0.0f) {
        wProjectionFlags.y = 1.0f;
      } else {
        wProjectionFlags.y = 0.0f;
      }
    } break;
    case 'p' : {
      if (wProjectionFlags.z == 0.0f) {
        wProjectionFlags.z = 1.0f;
      } else {
        wProjectionFlags.z = 0.0f;
      }
    } break;

    //case ']' : {
    //  loadShader();
    //} break;
  }
  glutPostRedisplay();
}

void Draw(void) {
  if (!g_shader)
    return;
    
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  g_shader->StartUsing();

  glUniform4fv(hCameraPosition, 1, _camera.getCameraPos().raw());
  //Mat4f transposedCamera = _camera.getCameraMatrix().transpose();
  //glUniformMatrix4fv(hCameraMatrix, 1, GL_FALSE, transposedCamera.raw());
  glUniformMatrix4fv(hCameraMatrix, 1, GL_TRUE, _camera.getCameraMatrix().raw());
  //Mat4f transposedFour = fourToThree.transpose();
  //glUniformMatrix4fv(hFourToThree, 1, GL_FALSE, transposedFour.raw());
  glUniformMatrix4fv(hFourToThree, 1, GL_TRUE, fourToThree.raw());
  glUniform4fv(hWPlaneNearFar, 1, wPlaneNearFar.raw());
  glUniform4fv(hProjectionFlags, 1, wProjectionFlags.raw());

  // fix the rotation to be smoother
  // figure out the clipping issues (negative w?)
  // normalize the input amounts
  // refactor the input system
  // switch the mat4 class to be column major (at least make the calls explicit)
  // multi-view rendering

  glUniformMatrix4fv(hWorldMatrix, 1, GL_FALSE, worldMatrix.raw());

  for (TVecQuaxol::iterator quax_it = g_quaxols.begin();
    quax_it != g_quaxols.end();
    ++quax_it) {
    
    const Quaxol& q = *quax_it;

    float shift_amount = 10.0f;
    Vec4f shift;
    shift.x = shift_amount * q.x;
    shift.y = shift_amount * q.y;
    shift.z = shift_amount * q.z;
    shift.w = shift_amount * q.w;
    glUniform4fv(hWorldPosition, 1, shift.raw());

    int tesseractTris = tesseract.getNumberTriangles();
    int startTriangle = 0;
    int endTriangle = tesseractTris;
    glBegin(GL_TRIANGLES);
    Vec4f a, b, c;
    int colorIndex = 0;
    for (int t = startTriangle; t < endTriangle && t < tesseractTris; t++) {
      glVertexAttrib4fv(hColor, colorArray[colorIndex].raw());
      tesseract.getTriangle(t, a, b, c);
      glVertex4fv(a.raw());
      glVertex4fv(b.raw());
      glVertex4fv(c.raw());
      if ((t+1) % 2 == 0) {
        colorIndex = (colorIndex + 1) % colorArray.size();
      }
    }
    glEnd();
  }

  g_shader->StopUsing();
  
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
  if (deltaX > threshold || deltaY > threshold) {
    return;
  }
  accumulatedMouseX += deltaX;
  accumulatedMouseY += deltaY;
  //printf("PassiveMotion x:%d y:%d accumX:%d accumY:%d\n", x, y, accumulatedMouseX, accumulatedMouseY);
}

void ApplyMouseMove() {
  static float moveAmount = 0.01f;
  if (accumulatedMouseX) {
    _camera.ApplyRotationInput(moveAmount * -accumulatedMouseX, Camera::FORWARD, Camera::RIGHT);
    accumulatedMouseX = 0;
  }

  if (accumulatedMouseY) {
    _camera.ApplyRotationInput(moveAmount * accumulatedMouseY, Camera::FORWARD, Camera::UP);
    accumulatedMouseY = 0;
  }
}

void OnIdle() {
  ApplyMouseMove();
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

  Mat4f threeMat;
//  threeMat.eigen().

  Shader::TestShaderHash();
  Camera::TestComponents();
}

// Soooo tacky!
//#define RUN_TESTS

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
