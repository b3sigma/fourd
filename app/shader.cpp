
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

namespace fd {

Shader::~Shader() {
  Release();
  s_test_shader_refs--;
}

void Shader::Release() {
  for (TVecShaderIds::iterator shadIt = _subShaders.begin();
      shadIt != _subShaders.end();
      ++shadIt) {
    glDeleteShader(*shadIt);
  }
  if(_programId != 0) {
    glDeleteProgram(_programId);
  }
  if(_uniforms) {
    handle_hash_destroy(_uniforms);
    _uniforms = NULL;
  }
  if(_attribs) {
    handle_hash_destroy(_attribs);
    _attribs = NULL;
  }
}

Shader::Shader() 
    : _programId(0)
    , _shaderType(0)
    , _attribs(NULL)
    , _uniforms(NULL) {
  s_test_shader_refs++;
}

bool Shader::LoadFromFile(const char* vertexFile, const char* pixelFile) {
  AddSubShader(vertexFile, GL_VERTEX_SHADER);
  AddSubShader(pixelFile, GL_FRAGMENT_SHADER);

  GLuint programId = glCreateProgram();
  if(programId == 0) {
    printf("Couldn't create program v:%s f:%s\n", vertexFile, pixelFile);
    return false;
  }

  for (TVecShaderIds::iterator shadIt = _subShaders.begin();
      shadIt != _subShaders.end();
      ++shadIt) {
    glAttachShader(programId, *shadIt);
  }

  glLinkProgram(programId);
  
  for (TVecShaderIds::iterator shadIt = _subShaders.begin();
      shadIt != _subShaders.end();
      ++shadIt) {
    glDetachShader(programId, *shadIt);
  }

  GLint linkSuccess;
  glGetProgramiv(programId, GL_COMPILE_STATUS, &linkSuccess);
  if(linkSuccess == GL_FALSE) {
    printf("Shader compilation failed v:%s f:\n", vertexFile, pixelFile);
    GLint errorLength;
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &errorLength);
    std::string error;
    error.resize(errorLength);
    glGetProgramInfoLog(programId, errorLength, NULL, (GLchar*)&error[0]);
    printf("error msg: %s\n", error.c_str());
    glDeleteProgram(programId);
    return false;
  }

  _programId = programId;
  _attribs = handle_hash_create();
  _uniforms = handle_hash_create();

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

  GLint compileSuccess;
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileSuccess);
  if(compileSuccess == GL_FALSE) {
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

  _subShaders.push_back(shaderId);

  return true;
}

void Shader::StartUsing() const {
  glUseProgram(_programId);
}

void Shader::StopUsing() const {
  glUseProgram(0);
}

//////////////////
// TShaderHash support

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

GLint Shader::getAttrib(const char* name) {
  assert(name != NULL);
  assert(_programId != 0);
  assert(_attribs != NULL);
  // Why do we assume GL isn't already doing something like this?
  // TODO: test whether this is a pre-optimization, dumbass.
  GLint handle = handle_hash_get(_attribs, name);
  if (handle != HANDLE_HASH_NULL)
    return handle;

  handle = glGetAttribLocation(_programId, name);
  if(handle == -1) {
    printf("Couldn't find attrib:%s\n", name);
    return handle;
  }

  handle_hash_add(_attribs, name, handle);
  return handle;
}

GLint Shader::getUniform(const char* name) {
  assert(name != NULL);
  assert(_programId != 0);
  assert(_uniforms != NULL);
  // Why do we assume GL isn't already doing something like this?
  // TODO: test whether this is a pre-optimization, dumbass.
  GLint handle = handle_hash_get(_uniforms, name);
  if (handle != HANDLE_HASH_NULL)
    return handle;

  handle = glGetUniformLocation(_programId, name);
  if(handle == -1) {
    printf("Couldn't find uniform:%s\n", name);
    return handle;
  }

  handle_hash_add(_uniforms, name, handle);
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

