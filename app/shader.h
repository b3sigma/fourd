#pragma once

#include <Windows.h>

#include "../stb/stb.h"
#include <GL/glew.h>
#include <string>
  
namespace fd {

  class Shader;

  stb_declare_hash(STB_noprefix, TShaderHash, shader_hash_,
    const char*, fd::Shader*);
  //stb_declare_hash(STB_noprefix, TShaderHash, shader_hash_,
  //  std::string, fd::Shader*);

  class Shader {
  protected:
    GLuint _id;
    GLenum _shader_type;

    static TShaderHash* s_pShaderhash;
    static int s_test_shader_refs; // Just a silly var for test cases, will probably remove soon.

    Shader() : _id(0), _shader_type(0) {}

    std::string _name;

  public:
    Shader(const std::string& name) : Shader() {
      _name = name;
      printf("shader created n:%s\n", _name.c_str());
      s_test_shader_refs++;
    }
    ~Shader() {
      printf("shader destroyed n:%s\n", _name.c_str());
      s_test_shader_refs--;
    }

    bool LoadFromFile(const char* path, GLenum shaderType);

    static bool TestShaderHash();
  };
}

