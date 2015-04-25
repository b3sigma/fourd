#pragma once

#include <Windows.h>

#include "../stb/stb.h"
#include <GL/glew.h>
#include <string>
#include <vector>
  
namespace fd {

  class Shader;

  stb_declare_hash(STB_noprefix, TShaderHash, shader_hash_,
    const char*, fd::Shader*);

  class Shader {
  protected:
    typedef std::vector<GLuint> TVecShaderIds;
    TVecShaderIds _subShaders;

    GLuint _programId;
    GLenum _shaderType;

    static TShaderHash* s_pShaderhash;

  public:
    Shader() : _programId(0), _shaderType(0) {
      s_test_shader_refs++;
    }
    ~Shader();

    bool AddSubShader(const char* filename, GLenum shaderType);
    bool LoadFromFile(const char* vertexFile, const char* pixelFile);
    void Release();

  public:
    static bool TestShaderHash();
  private:
    static int s_test_shader_refs; // Just a silly var for test cases, will probably remove soon.

  };
}

