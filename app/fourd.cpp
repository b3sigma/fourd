#include <math.h>

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
// shared uniform buffer (something something sb140?)
// look like 56 floats would apply so far
// doing things so inefficiently already that much more refactoring necessary
GLint position;
GLint color;
GLint cgWorldMatrix;
GLint cgWorldPosition;
GLint cgCameraPosition;
GLint cgCameraMatrix;
GLint cgProjectionMatrix;
GLint cgFourToThree;
GLint cgWPlaneNearFar;

// TODO: Move most of the CG/GL code to render, including the camera
// Setup a render target approach
// Get multi-view working
// Get a post effect working
// Integrate rift

Mat4f worldMatrix;
Mat4f projectionMatrix;
Mat4f fourToThree;
Vec4f wPlaneNearFar;
Mesh tesseract;
Camera _camera;
float _fov = 45.0f;
float _near = 0.1f;
float _far = 100000.0f;
int _width = 800;
int _height = 600;
int cubeIndex = 0;
::fd::Render renderer;
::fd::TVecQuaxol quaxols_g;
::fd::Texture g_texture;
::fd::Shader g_shader;
// trying out different naming conventions ok? quit complaining

typedef std::vector<Vec4f> VectorList;
VectorList colorArray;
void buildColorArray() {
  int numSteps = 8;
  colorArray.reserve(numSteps);
  for (int steps = 0; steps < numSteps; steps++) {
    colorArray.push_back(Vec4f(0, (float)(steps + 1) / (float)numSteps, 0, 1));
  }
}

bool loadShader() {
  g_shader.Release();
  if(!g_shader.LoadFromFile(
      "data\\vertFourd.glsl", "data\\fragFourd.glsl")) {
    printf("Failed loading shader!\n");
    return false;
  }

  g_shader.StartUsing();
  position = g_shader.getAttrib("vertPosition");
  color = g_shader.getAttrib("vertColor");
  cgWorldMatrix = g_shader.getUniform("worldMatrix");
  cgWorldPosition = g_shader.getUniform("worldPosition");
  cgCameraPosition = g_shader.getUniform("cameraPosition");
  cgCameraMatrix = g_shader.getUniform("cameraMatrix");
  cgProjectionMatrix = g_shader.getUniform("projectionMatrix");
  cgFourToThree = g_shader.getUniform("fourToThree");
  cgWPlaneNearFar = g_shader.getUniform("wPlaneNearFar");
  g_shader.StopUsing();

  return true;
}

bool LoadLevel() {
  ChunkLoader chunks;
  if (chunks.LoadFromFile("data\\level.txt")) {
    std::swap(quaxols_g, chunks.quaxols_);
    printf("Level loaded %d quaxols!\n", (int)quaxols_g.size());
    return true;
  }
  else {
    printf("Couldn't load the level!\n");
    return false;
  }
}

bool Initialize() {
  //tesseract.buildQuad(10.0f, Vec4f(-20.0, 0, -20.0, 0));
  //tesseract.buildCube(10.0f, Vec4f(0, 0, 0, 0));
  //tesseract.buildTesseract(10.0f, Vec4f(0,0,0,0.0f), Vec4f(0,0,0,0));
  tesseract.buildTesseract(10.0f, Vec4f(0.1f,0.1f,0.1f,0.1f), Vec4f(0,0,0,0));
  _camera.setMovementMode(Camera::MovementMode::LOOK); //ORBIT); //LOOK);
  wPlaneNearFar.z = 1.0f; // use projective 4d mode
  _camera.SetCameraPosition(Vec4f(0.5f, 0.5f, 50.5f, 10.0f));

  LoadLevel();
  
  g_texture.LoadFromFile("data\\orientedTexture.png");

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0f);
  glDepthFunc(GL_ALWAYS); //GL_LEQUAL);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  // So apparently glBlendEquation just didn't get included in msvc.
  // Need to include the entire glew project just to get it work??
  //glBlendEquation(GL_ADD);
  //glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA); //GL_ONE_MINUS_SRC_ALPHA); // 
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 
  glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //GL_FILL); //GL_LINE);

  //cgContext = cgCreateContext();
  //if (cgContext == 0) {
  //  fprintf(stdio, "Failed To Create Cg Context\n");
  //  exit(-1);
  //}

  if (!loadShader()) {
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

  buildColorArray();

  return true;
}

void Deinitialize(void) {
  //cgDestroyContext(cgContext);
}

void UpdatePerspective() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float aspect = static_cast<float>(_width) / static_cast<float>(_height);
  projectionMatrix.build3dProjection(_fov, aspect, _near, _far);
  g_shader.StartUsing();
  glUniformMatrix4fv(cgProjectionMatrix, 1, GL_FALSE, projectionMatrix.raw());
  g_shader.StopUsing();
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
      wPlaneNearFar.z -= 1.0f;
      //_fov -= 5.0f;
      //UpdatePerspective();
    } break;
    case 'm' : {
      wPlaneNearFar.z += 1.0f;
      //_fov += 5.0f;
      //UpdatePerspective();
    } break;
    case '?' : {
      _camera.printIt();
      printf("\nwPlaneNearFar\n");
      wPlaneNearFar.printIt();
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
      static float storedFarZ = 1.0f;
      if (wPlaneNearFar.z == 0.0f) {
        wPlaneNearFar.z = storedFarZ;
      } else {
        storedFarZ = wPlaneNearFar.z;
        wPlaneNearFar.z = 0.0f;
      }
    } break;
    case 'p': {
      LoadLevel();
    } break;

    //case ']' : {
    //  loadShader();
    //} break;
  }
  glutPostRedisplay();
}

void Draw(void) {
    
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  g_shader.StartUsing();

  //position = g_shader.getAttrib("vertPosition");
  //color = g_shader.getAttrib("vertColor");
  //cgWorldMatrix = g_shader.getUniform("worldMatrix");
  //cgWorldPosition = g_shader.getUniform("worldPosition");
  //cgCameraPosition = g_shader.getUniform("cameraPosition");
  //cgCameraMatrix = g_shader.getUniform("cameraMatrix");
  //cgProjectionMatrix = g_shader.getUniform("projectionMatrix");
  //cgFourToThree = g_shader.getUniform("fourToThree");
  //cgWPlaneNearFar = g_shader.getUniform("wPlaneNearFar");

  glUniform4fv(cgCameraPosition, 1, _camera.getCameraPos().raw());
  Mat4f transposedCamera = _camera.getCameraMatrix().transpose();

  glUniformMatrix4fv(cgCameraMatrix, 1, GL_FALSE, transposedCamera.raw());
  Mat4f transposedFour = fourToThree.transpose();
  glUniformMatrix4fv(cgFourToThree, 1, GL_FALSE, transposedFour.raw());
  glUniform4fv(cgWPlaneNearFar, 1, wPlaneNearFar.raw());

  // fix the rotation to be smoother
  // figure out the clipping issues (negative w?)
  // normalize the input amounts
  // refactor the input system
  // switch the mat4 class to be column major (at least make the calls explicit)
  // multi-view rendering

  //Vec4f shift(0.0f, 0.0f, 10.0f * sin(renderer.GetFrameTime()), 0.0f);

  glUniformMatrix4fv(cgWorldMatrix, 1, GL_FALSE, worldMatrix.raw());

  //cgGLEnableProfile(cgVertexProfile);
  //cgGLBindProgram(cgProgram);

  for (TVecQuaxol::iterator quax_it = quaxols_g.begin();
    quax_it != quaxols_g.end();
    ++quax_it) {
    
    const Quaxol& q = *quax_it;

    float shift_amount = 10.0f;
    Vec4f shift;
    shift.x = shift_amount * q.x;
    shift.y = shift_amount * q.y;
    shift.z = shift_amount * q.z;
    shift.w = shift_amount * q.w;
    glUniform4fv(cgWorldPosition, 1, shift.raw());

    int tesseractTris = tesseract.getNumberTriangles();
    int startTriangle = 0;
    int endTriangle = tesseractTris;
    glBegin(GL_TRIANGLES);
    Vec4f a, b, c;
    int colorIndex = 0;
    for (int t = startTriangle; t < endTriangle && t < tesseractTris; t++) {
      glVertexAttrib4fv(color, colorArray[colorIndex].raw());
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

  g_shader.StopUsing();
  //cgGLDisableProfile(cgVertexProfile);
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
}

#define DERP_FUCKTARD
#ifndef DERP_FUCKTARD

int main(int argc, char *argv[]) {
  RunTests();
  
  glutInit(&argc, argv);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(640, 480);
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

#else 

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

	char vs[] = R"foo(
varying vec3 normal;
uniform vec4 worldPosition;
void main()
{	
  normal = gl_Normal;
	gl_Position = ftransform() + worldPosition;
}
)foo";
	char fs[] = R"foo(
varying vec3 normal;
void main()
{
	vec4 color;
  color = vec4(1.0,0.2,0.1,1.0);
  color.xyz += normal; 
	gl_FragColor = color;
}
)foo";
  const char* vsPtr = &vs[0];
  const char* fsPtr = &fs[0];

	glShaderSource(v, 1, &vsPtr,NULL);
	glShaderSource(f, 1, &fsPtr,NULL);

	glCompileShader(v);
	glCompileShader(f);
	
	p = glCreateProgram();
	glAttachShader(p,f);
	glAttachShader(p,v);

	glLinkProgram(p);
	glUseProgram(p);
  glUseProgram(0);
}
#else // DERP_INLINE_SHADERS
fd::Shader derpShader;
#endif // DERP_INLINE_SHADERS

void derpRenderScene(void) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  static float timey = 0.0f;
  timey += 0.008f;
  float movey = sinf(timey) * 1.0f;
  char uniformName[] = "worldPosition";
#ifdef DERP_INLINE_SHADERS
  glUseProgram(p);
  GLint handle = glGetUniformLocation(p, uniformName);
  if (handle == -1) {
    printf ("couldn't get handle %s\n", uniformName);
  } else {
    Vec4f moveIt(movey, movey, movey, 1.0f);
    glUniform4fv(handle, 1, moveIt.raw());
  }
  glUseProgram(0);
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
  derpShader.StopUsing();
#endif //def DERP_INLINE_SHADERS

	glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(-0.5,-0.5,0.0);
    glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3f(0.5,0.0,0.0);
    glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3f(0.0,0.5,0.0);
	glEnd();

  glLoadIdentity();
	gluLookAt(0.0,0.0,5.0, 
  		      0.0,0.0,-1.0,
	    		  0.0f,1.0f,0.0f);

	float lpos[4] = {1,0.5,1,0};
  glLightfv(GL_LIGHT0, GL_POSITION, lpos);

#ifdef DERP_INLINE_SHADERS
  glUseProgram(p);
	glutSolidTeapot(1);
  glUseProgram(0);
#else
  derpShader.StartUsing();
	glutSolidTeapot(1);
  derpShader.StopUsing();
#endif //def DERP_INLINE_SHADERS

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
  
  glewExperimental=TRUE;
  if(glewInit() != GLEW_OK) {
    printf("glew init fail\n");
    return false;
  }

  glutReshapeFunc(derpReshapeGL);
  glutKeyboardFunc(derpKey);
  glutDisplayFunc(derpRenderScene);
  glutIdleFunc(derpIdle);

  Initialize();

#ifdef DERP_INLINE_SHADERS
  loadInlineShaders();
#else
  derpShader.Release();
  derpShader.LoadFromFile("data\\vertTrivial.glsl", "data\\fragTrivial.glsl");
#endif
  g_texture.LoadFromFile("data\\orientedTexture.png");


  glutMainLoop();
  return 0;
}

#endif // DERP_FUCKTARD

