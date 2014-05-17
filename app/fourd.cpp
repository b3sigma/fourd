#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include "../common/fourmath.h"
#include "../common/mesh.h"
#include "../common/camera.h"
#include "render.h"

using namespace ::fd;

CGcontext cgContext;
CGprogram cgProgram;
CGprofile cgVertexProfile;
CGparameter position;
CGparameter color;
CGparameter cgWorldMatrix;
CGparameter cgWorldPosition;
CGparameter cgCameraPosition;
CGparameter cgCameraMatrix;
CGparameter cgProjectionMatrix;
CGparameter cgFourToThree;

// TODO: Move most of the CG/GL code to render, including the camera
// Setup a render target approach
// Get multi-view working
// Get a post effect working
// Integrate rift

Mat4f worldMatrix;
Mat4f projectionMatrix;
Mat4f fourToThree;
Mesh tesseract;
Camera _camera;
float _fov = 90.0f;
float _near = 0.1f;
float _far = 100000.0f;
int _width = 800;
int _height = 600;
int cubeIndex = 0;

typedef std::vector<Vec4f> VectorList;
VectorList colorArray;
void buildColorArray() {
  int numSteps = 8;
  colorArray.reserve(numSteps);
  for (int steps = 0; steps < numSteps; steps++) {
    colorArray.push_back(Vec4f(0, (float)(steps + 1) / (float)numSteps, 0, 1));
  }
}


bool Initialize() {
  //tesseract.buildQuad(10.0f, Vec4f(-20.0, 0, -20.0, 0));
  //tesseract.buildCube(10.0f, Vec4f(0, 0, 0, 0));
  tesseract.buildTesseract(10.0f, Vec4f(0,0,0,0), Vec4f(0, 1, 2, 0));
  _camera.SetCameraPosition(Vec4f(0.5f, 0.5f, 60.5f, 0.0f));

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0f);
  glDepthFunc(GL_ALWAYS); //GL_LEQUAL);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendEquation(GL_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
  glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //GL_FILL); //GL_LINE);

  cgContext = cgCreateContext();

  if (cgContext == 0) {
    fprintf(stderr, "Failed To Create Cg Context\n");
    exit(-1);
  }

  cgVertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
  if (cgVertexProfile == CG_PROFILE_UNKNOWN) {
    fprintf(stderr, "Invalid profile type\n");
    exit(-1);
  }

  cgGLSetOptimalOptions(cgVertexProfile);

  cgProgram = cgCreateProgramFromFile(cgContext, CG_SOURCE, "./cg/four.cg",
      cgVertexProfile, "main", 0);

  if (cgProgram == 0) {
    CGerror Error = cgGetError();

    fprintf(stderr, "%s \n", cgGetErrorString(Error));
    exit(-1);
  }

  cgGLLoadProgram(cgProgram);

  position = cgGetNamedParameter(cgProgram, "IN.position");
  color = cgGetNamedParameter(cgProgram, "IN.color");
  cgWorldMatrix = cgGetNamedParameter(cgProgram, "worldMatrix");
  cgWorldPosition = cgGetNamedParameter(cgProgram, "worldPosition");
  cgCameraPosition = cgGetNamedParameter(cgProgram, "cameraPosition");
  cgCameraMatrix = cgGetNamedParameter(cgProgram, "cameraMatrix");
  cgProjectionMatrix = cgGetNamedParameter(cgProgram, "projectionMatrix");
  cgFourToThree = cgGetNamedParameter(cgProgram, "fourToThree");

  worldMatrix.storeIdentity();
  projectionMatrix.storeIdentity();
  fourToThree.storeIdentity();
  float nearInside = 1.0f;
  //float farInside = 1000.0f;
  fourToThree[0][0] = nearInside;
  fourToThree[1][1] = nearInside;
  fourToThree[2][2] = nearInside;

  buildColorArray();

  return true;
}

void Deinitialize(void) {
  cgDestroyContext(cgContext);
}

void UpdatePerspective() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(_fov, (GLfloat) (_width) / (GLfloat) (_height), _near, _far);
  cgGLSetStateMatrixParameter(cgProjectionMatrix, CG_GL_PROJECTION_MATRIX,
      CG_GL_MATRIX_IDENTITY);
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
  static float rollAmount = moveAmount * 2 * PI / 100.0f;

  switch (key) {
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
    case '9' : {
      static bool fill = false;
      fill = !fill;
      if (fill) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      }
    } break;
    case 27: {
      Deinitialize();
      exit(0);
    }
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
      _camera.ApplyTranslationInput(-moveAmount, Camera::INSIDE);
    } break;
    case 'f' : {
      _camera.ApplyTranslationInput(moveAmount, Camera::INSIDE);
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
    // Sure this looks like an unsorted mess, but is spatially aligned kinda.
    case 'x' : {
      _near *= 0.1f;
      UpdatePerspective();
    } break;
    case 'c' : {
      _near *= 10.0f;
      UpdatePerspective();
    } break;
    case 'v' : {
      _far *= 0.1f;
      UpdatePerspective();
    } break;
    case 'b' : {
      _far *= 10.0f;
      UpdatePerspective();
    } break;
    case 'n' : {
      _fov -= 5.0f;
      UpdatePerspective();
    } break;
    case 'm' : {
      _fov += 5.0f;
      UpdatePerspective();
    } break;
    case '?' : {
      _camera.printIt();
    } break;
    case '\'' : {
      tesseract.printIt();
    } break;
  }
  glutPostRedisplay();
}

void Draw(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  cgGLSetParameter4fv(cgCameraPosition, _camera.getCameraPos().raw());
  cgGLSetMatrixParameterfc(cgWorldMatrix, worldMatrix.raw());
  Mat4f transposedCamera = _camera.getCameraMatrix().transpose();
  cgGLSetMatrixParameterfc(cgCameraMatrix, transposedCamera.raw());
  Mat4f transposedFour = fourToThree.transpose();
  cgGLSetMatrixParameterfc(cgFourToThree, transposedFour.raw());

  // fix the rotation to be smoother
  // figure out the clipping issues (negative w?)
  // normalize the input amounts
  // refactor the input system
  // switch the mat4 class to be column major (at least make the calls explicit)
  // multi-view rendering


  cgGLEnableProfile(cgVertexProfile);
  cgGLBindProgram(cgProgram);

  static bool drawTesseract = true;
  if (drawTesseract) {
    int tesseractTris = tesseract.getNumberTriangles();
    int startTriangle = 0;
    int endTriangle = tesseractTris;
    glBegin(GL_TRIANGLES);
    Vec4f a, b, c;
    int colorIndex = 0;
    for (int t = startTriangle; t < endTriangle && t < tesseractTris; t++) {
      cgGLSetParameter4fv(color, colorArray[colorIndex].raw());
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

  cgGLDisableProfile(cgVertexProfile);
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

  Mat4f rotXFourth;
  rotXFourth.buildRotation(PI / 2.0, 1, 0);
  Mat4f rotXEighth;
  rotXEighth.buildRotation(PI / 4.0, 1, 0);
  assert(iden == (rotXFourth * rotXFourth * rotXFourth * rotXFourth * iden));
  assert(rotXEighth * rotXEighth == rotXFourth);
  assert(!(rotXFourth * rotXFourth == rotXFourth));
}

int main(int argc, char *argv[]) {
  RunTests();

  glutInit(&argc, argv);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(640, 480);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow(argv[0]);
  glutReshapeFunc(ReshapeGL);
  glutKeyboardFunc(Key);
  glutMouseFunc(MouseClick);
  glutPassiveMotionFunc(PassiveMotion);
  glutMotionFunc(Motion);
  glutSpecialFunc(Update);
  glutDisplayFunc(Draw);
  glutIdleFunc(OnIdle);
  Initialize();
  glutMainLoop();
  return 0;
}

