#pragma once

#ifdef WIN32
#include <Windows.h>
#endif //WIN32

#include "../stb/stb.h"
#include <GL/glew.h>
#include <string>
#include <vector>
  
namespace fd {

  class Camera;
  class Shader;

  stb_declare_hash(STB_noprefix, TShaderHash, shader_hash_,
    const char*, fd::Shader*);

  stb_declare_hash(STB_noprefix, THandleHash, handle_hash_,
    const char*, GLint);

  class Shader {
  protected:
    typedef std::vector<GLuint> TVecShaderIds;
    TVecShaderIds m_subShaders;
    std::string m_refName;

    GLuint m_programId;
    GLenum m_shaderType;

    THandleHash* m_attribs;
    THandleHash* m_uniforms;

    static TShaderHash* s_pShaderhash;

    // because msvc can't do constexpr quite right, must update cpp
    // names if you change these in InitCameraParamHandles
    enum cameraShaderHandleEnum {
      ECameraPosition,
      ECameraMatrix,
      EProjectionMatrix,
      EFourToThree,
      EWPlaneNearFar,
      ENumCameraShaderHandles,
    };
    GLint m_cameraHandles[ENumCameraShaderHandles];

  public:
    Shader();
    ~Shader();

    void InitCameraParamHandles();
    void SetCameraParams(Camera* pCamera);

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

