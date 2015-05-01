
#ifdef WIN32
#include <Windows.h>
#endif // WIN32


#define STB_DEFINE
#pragma warning(push)
#pragma warning(disable: 4996)  // disable warning about _s functions
#include "../stb/stb.h"
#pragma warning(pop)

#define FD_SIMPLE_FILE_IMPLEMENTATION
#include "../common/fd_simple_file.h"

#include "shader.h"
#include "../common/camera.h"

namespace fd {

Shader::~Shader() {
  RemoveFromShaderHash();
  Release();
  s_test_shader_refs--;
}

void Shader::Release() {
  for (auto shaderId : m_subShaders) {
    glDeleteShader(shaderId);
  }
  m_subShaders.resize(0);
  if(m_programId != 0) {
    glDeleteProgram(m_programId);
    m_programId = 0;
  }
  if(m_uniforms) {
    handle_hash_destroy(m_uniforms);
    m_uniforms = NULL;
  }
  if(m_attribs) {
    handle_hash_destroy(m_attribs);
    m_attribs = NULL;
  }
}

Shader::Shader() 
    : m_programId(0)
    , m_shaderType(0)
    , m_attribs(NULL)
    , m_uniforms(NULL) {
  s_test_shader_refs++;
}


void Shader::InitCameraParamHandles() {
  assert(m_programId != 0);
  m_cameraHandles[ECameraPosition] = getUniform("cameraPosition");
  m_cameraHandles[ECameraMatrix] = getUniform("cameraMatrix");
  m_cameraHandles[EProjectionMatrix] = getUniform("projectionMatrix");
  m_cameraHandles[EFourToThree] = getUniform("fourToThree");
  m_cameraHandles[EWPlaneNearFar] = getUniform("wPlaneNearFar");
}

void Shader::SetCameraParams(Camera* pCamera) {  
  assert(pCamera != NULL);

  // This global state with using is error prone
  //StartUsing();
  glUniform4fv(m_cameraHandles[ECameraPosition], 1, 
      pCamera->getCameraPos().raw());
  glUniformMatrix4fv(m_cameraHandles[ECameraMatrix], 1, GL_TRUE, 
      pCamera->getCameraMatrix().raw());
  // Sure is weird that this one isn't transposed...
  // Thinking we are doing inconsistent row/col in the projection creation
  glUniformMatrix4fv(m_cameraHandles[EProjectionMatrix], 1, GL_FALSE, 
      pCamera->_zProjectionMatrix.raw());

  // This isn't even used so far..
  glUniformMatrix4fv(m_cameraHandles[EFourToThree], 1, GL_TRUE,
      pCamera->_fourToThree.raw());
  // As the calculation of wNear/wFar is happening manually
  // Dunno if these param packing things are worthwhile
  Vec4f wPlaneNearFar(pCamera->_wNear, pCamera->_wFar, pCamera->_wScreenSizeRatio, 0.0f);
  glUniform4fv(m_cameraHandles[EWPlaneNearFar], 1, wPlaneNearFar.raw());
  //StopUsing();
}

void Shader::ClearShaderHash() {
  // um fuck? seems to be no way to iterate through this stupid stb hash
  // TODO: change to a decent hash or unordered_map
}

Shader* Shader::GetShaderByRefName(const char* refName) {
  return (s_pShaderhash != NULL) ?
      shader_hash_get(s_pShaderhash, refName) : NULL;
}

void Shader::AddToShaderHash() {
  if (s_pShaderhash == NULL) {
    s_pShaderhash = shader_hash_create();
  }

  if (NULL == shader_hash_get(s_pShaderhash, m_refName.c_str())) {
    shader_hash_add(s_pShaderhash, m_refName.c_str(), this);
  }
}

void Shader::RemoveFromShaderHash() {
  if( s_pShaderhash) {
    shader_hash_remove(s_pShaderhash, m_refName.c_str(), NULL);
  }
}

bool Shader::LoadFromFile(const char* refName, 
    const char* vertexFile, const char* pixelFile) {
  AddSubShader(vertexFile, GL_VERTEX_SHADER);
  AddSubShader(pixelFile, GL_FRAGMENT_SHADER);

  GLuint programId = glCreateProgram();
  if(programId == 0) {
    printf("Couldn't create program v:%s f:%s\n", vertexFile, pixelFile);
    return false;
  }

  for (auto shaderId : m_subShaders) {
    glAttachShader(programId, shaderId);
  }

  glLinkProgram(programId);
  
  for (auto shaderId : m_subShaders) {
    glDetachShader(programId, shaderId);
  }

  GLint linkSuccess = GL_TRUE;
  glGetProgramiv(programId, GL_COMPILE_STATUS, &linkSuccess);
  if(linkSuccess != GL_TRUE) {
    printf("program linking failed v:%s f:%s err:%d\n",
        vertexFile, pixelFile, linkSuccess);
    GLint errorLength;
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &errorLength);
    std::string error;
    error.resize(errorLength);
    glGetProgramInfoLog(programId, errorLength, NULL, (GLchar*)&error[0]);
    printf("error msg: %s\n", error.c_str());
    glDeleteProgram(programId);
    return false;
  }

  m_programId = programId;
  m_attribs = handle_hash_create();
  m_uniforms = handle_hash_create();
  m_refName.assign(refName);
  AddToShaderHash();
  InitCameraParamHandles();

  return true;
}

bool Shader::AddSubShader(const char* filename, GLenum shaderType) {
  std::string buffer;
  if(!fd_file_to_string(filename, buffer))
  {
    printf("Couldn't read shader file:%s\n", filename);
    return false;
  }

  GLuint shaderId = glCreateShader(shaderType);
  if(shaderId == 0)
    return false;

  const char* bufferString = buffer.c_str();
  glShaderSource(shaderId, 1, (const GLchar* const*)&(bufferString), NULL);
  glCompileShader(shaderId);

  GLint compileSuccess = GL_TRUE;
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileSuccess);
  if(compileSuccess != GL_TRUE) {
    printf("Shader compilation failed:%s\ndump:%s\n", filename, buffer.c_str());
    GLint errorLength;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &errorLength);
    std::string error;
    error.resize(errorLength);
    glGetShaderInfoLog(shaderId, errorLength, NULL, (GLchar*)&error[0]);
    printf("error msg: %s\n", error.c_str());
    glDeleteShader(shaderId);
    return false;
  }

  m_subShaders.push_back(shaderId);

  return true;
}

void Shader::StartUsing() const {
  assert(GetIsUsing() == false);
  glUseProgram(m_programId);
  assert(GetIsUsing() == true);
}

void Shader::StopUsing() const {
  assert(GetIsUsing() == true);
  glUseProgram(0);
  assert(GetIsUsing() == false);
}

bool Shader::GetIsUsing() const {
  if (m_programId <= 0) return false;
  GLint currentProgram = -1;
  glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
  return (currentProgram == m_programId);
}

//////////////////
// TShaderHash support
// This stb hash business was a massive hassle to get to compile,
// and the other huge downwside
// of single file header libraries that are #define based shows up when
// there is a problem and debugging is essentially guesswork.

TShaderHash* Shader::s_pShaderhash = NULL;

#define STR_KEY_HASH_EMPTY ((const char*)(1))
#define STR_KEY_HASH_DEL ((const char*)(2))

// Ugh, this is silly. Would rather std::string but stb hash is malloc based
// so it didn't call constructors.
const char* string_key_hash_str_copy(const char* str) {
  if (str == NULL) return NULL;
  if (str == STR_KEY_HASH_EMPTY) return STR_KEY_HASH_EMPTY;
  if (str == STR_KEY_HASH_DEL) return STR_KEY_HASH_DEL;

  unsigned int size = strlen(str) + 1;
  char* newStr = new char[size];
  strcpy_s(newStr, size, str);
  return newStr;
}

void string_key_hash_str_del(const char* str) {
  delete [] str;
}

bool string_key_hash_str_cmp(const char* left, const char* right) {
  if (left == right) { return true; }
  if (left == NULL || left == STR_KEY_HASH_EMPTY || left == STR_KEY_HASH_DEL ||
      right == NULL || right == STR_KEY_HASH_EMPTY || right == STR_KEY_HASH_DEL) {
    return false;
  }

  return _stricmp(left, right) == 0;
}

stb_define_hash_base(STB_noprefix, TShaderHash, STB_nofields,
  shader_hash_, shader_hash_, 0.85f, const char*, 
  STR_KEY_HASH_EMPTY, STR_KEY_HASH_DEL, 
  string_key_hash_str_copy, string_key_hash_str_del, STB_nosafe,
  string_key_hash_str_cmp, string_key_hash_str_cmp,
  return stb_hash(k); , Shader*, STB_nullvalue, NULL);

//////////////////
// THandleHash support

#define HANDLE_HASH_NULL ((GLint)(-1))

stb_define_hash_base(STB_noprefix, THandleHash, STB_nofields,
  handle_hash_, handle_hash_, 0.85f, const char*, 
  STR_KEY_HASH_EMPTY, STR_KEY_HASH_DEL, 
  string_key_hash_str_copy, string_key_hash_str_del, STB_nosafe,
  string_key_hash_str_cmp, string_key_hash_str_cmp,
  return stb_hash(k); , GLint, STB_nullvalue, HANDLE_HASH_NULL);

GLint Shader::getAttrib(const char* name) const {
  assert(name != NULL);
  assert(m_programId != 0);
  assert(m_attribs != NULL);
  // Why do we assume GL isn't already doing something like this?
  // TODO: test whether this is a pre-optimization, dumbass.
  GLint handle = handle_hash_get(m_attribs, name);
  if (handle != HANDLE_HASH_NULL)
    return handle;

  handle = glGetAttribLocation(m_programId, name);
  if(handle == -1) {
    printf("Couldn't find attrib:%s\n", name);
    return handle;
  }

  handle_hash_add(m_attribs, name, handle);
  return handle;
}

GLint Shader::getUniform(const char* name) const {
  assert(name != NULL);
  assert(m_programId != 0);
  assert(m_uniforms != NULL);
  // Why do we assume GL isn't already doing something like this?
  // TODO: test whether this is a pre-optimization, dumbass.
  GLint handle = handle_hash_get(m_uniforms, name);
  if (handle != HANDLE_HASH_NULL)
    return handle;

  handle = glGetUniformLocation(m_programId, name);
  if(handle == -1) {
    printf("Couldn't find uniform:%s\n", name);
    return handle;
  }

  handle_hash_add(m_uniforms, name, handle);
  return handle;
}

/////////////////
// TShaderHash test

int Shader::s_test_shader_refs = 0;

bool Shader::TestShaderHash() {
  TShaderHash* pTestHash = shader_hash_create();

  int startingShaderRefs = Shader::s_test_shader_refs;

  char hashTest1[] = "shady0";
  char hashTest2[] = "shady1";
  assert(stb_hash(&hashTest1[0]) != stb_hash(&hashTest2[0]));
  
  int numEntries = 100;
  for (int test = 0; test < numEntries; ++test) {
    char* name = new char[256];
    sprintf_s(&(name[0]), sizeof(char[256]), "shady%d", test);
    Shader* pShader = new Shader();
    
    assert(0 == shader_hash_update(pTestHash, name, pShader));
    assert(1 == shader_hash_add(pTestHash, name, pShader));
    assert(0 == shader_hash_add(pTestHash, name, pShader));
    //assert(1 == shader_hash_set(pTestHash, name, pShader));
    //assert(1 == shader_hash_update(pTestHash, name, pShader));

    assert(shader_hash_get(pTestHash, name) == pShader);
    assert(shader_hash_get(pTestHash, std::string(name).c_str()) == pShader);
  }

  assert(Shader::s_test_shader_refs == (numEntries + startingShaderRefs));

  for (int test = 0; test < numEntries; ++test) {
    int nameIndex = rand() + numEntries;
    char* name = new char[256];
    sprintf_s(&(name[0]), sizeof(char[256]), "shady%d", nameIndex);
    Shader* pShader = new Shader();

    if(1 == shader_hash_add(pTestHash, name, pShader)) {
      assert(shader_hash_get(pTestHash, name) == pShader);
      
      Shader* pToDelete = NULL;
      shader_hash_remove(pTestHash, std::string(name).c_str(), &pToDelete);
      assert(pShader == pToDelete);
    }
    delete pShader;
  }

  assert(Shader::s_test_shader_refs == (numEntries + startingShaderRefs));

  for (int test = 0; test < numEntries; ++test) {
    char* name = new char[256];
    sprintf_s(&(name[0]), sizeof(char[256]), "shady%d", test);

    Shader* pShader = shader_hash_get(pTestHash, std::string(name).c_str());

    Shader* pToDelete = NULL;
    shader_hash_remove(pTestHash, std::string(name).c_str(), &pToDelete);
    delete pToDelete;
    delete [] name;
  }

  assert(Shader::s_test_shader_refs == startingShaderRefs);
  shader_hash_destroy(pTestHash);

  return true;
}



}

