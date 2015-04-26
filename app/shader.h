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

  stb_declare_hash(STB_noprefix, THandleHash, handle_hash_,
    const char*, GLint);

  class Shader {
  protected:
    typedef std::vector<GLuint> TVecShaderIds;
    TVecShaderIds _subShaders;
    std::string _refName;

    GLuint _programId;
    GLenum _shaderType;

    THandleHash* _attribs;
    THandleHash* _uniforms;

    static TShaderHash* s_pShaderhash;

  public:
    Shader();
    ~Shader();

    bool AddSubShader(const char* filename, GLenum shaderType);
    bool LoadFromFile(const char* refName, const char* vertexFile, const char* pixelFile);
    void Release();

    void StartUsing() const;
    void StopUsing() const;
    bool GetIsUsing() const;

    GLint getAttrib(const char* name) const;
    GLint getUniform(const char* name) const;

    static void ClearShaderHash();
    static Shader* GetShaderByRefName(const char* refName);
    
  protected:
    void AddToShaderHash();
    void RemoveFromShaderHash();

  public:
    static bool TestShaderHash();
  private:
    static int s_test_shader_refs; // Just a silly var for test cases, will probably remove soon.

  };
}

