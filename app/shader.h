#pragma once

#ifdef WIN32
#include <Windows.h>
#endif //WIN32

//#include "../stb/stb.h"
#include <GL/glew.h>
#include <string>
#include <vector>
#include <unordered_map>

#include "../common/fourmath.h"
  
namespace fd {

  class Camera;
  class Shader;

  //stb_declare_hash(STB_noprefix, TShaderHash, shader_hash_,
  //  const char*, fd::Shader*);

  //stb_declare_hash(STB_noprefix, THandleHash, handle_hash_,
  //  const char*, GLint);

  class Shader {
  protected:
    typedef std::vector<GLuint> TVecShaderIds;
    TVecShaderIds m_subShaders;
    std::string m_refName;

    typedef std::vector<std::pair<std::string, GLenum>> TSubShaderNames;
    TSubShaderNames m_subShaderNames;

    GLuint m_programId;
    GLenum m_shaderType;

    //THandleHash* m_attribs;
    //THandleHash* m_uniforms;

    typedef std::unordered_map<std::string, Shader*> ShaderHash;
    static ShaderHash s_shaderhash;

  public:
    // because msvc can't do constexpr quite right, must update cpp
    // names if you change these in InitCameraParamHandles
    enum shaderHandleEnum {
      // attribs
      AVertPosition,
      AVertColor,
      AVertBoneIndex,

      // uniforms, per object
      UWorldMatrix,
      UWorldPosition,
      UTexDiffuse0,

      // uniforms, skinning, per object
      UBoneRotations,
      UBonePositions,

      // uniforms, camera
      UCameraPosition,
      UCameraMatrix,
      UProjectionMatrix,
      UFourToThree,
      UWPlaneNearFar,

      // accounting
      ENumCameraShaderHandles,
    };
    GLint m_handles[ENumCameraShaderHandles];

  public:
    Shader();
    ~Shader();

    void InitCameraParamHandles();
    void SetCameraParams(const Camera* pCamera) const; // const but gl side effects...
    void SetOrientation(const Mat4f* pOrientation) const;
    void SetPosition(const Vec4f* pPosition) const;
    GLint GetColorHandle() const;

    bool AddDynamicMeshCommonSubShaders();
    bool AddSubShader(const char* filename, GLenum shaderType);
    bool LoadFromFileDerivedNames(const char* refName);
    bool LoadFromFile(const char* refName, const char* vertexFile, const char* pixelFile);
    void Release();
    bool Reload();

    void StartUsing() const;
    void StopUsing() const;
    bool GetIsUsing() const;

    GLint getProgramId() const { return m_programId; }
    GLint getAttrib(const char* name) const;
    GLint getUniform(const char* name) const;

    static bool CheckGLShaderCompileStatus(
        GLuint shaderId, const char* filename);
    static bool CheckGLShaderLinkStatus(
        GLuint programId, const char* refName);

    static void ClearShaderHash();
    static Shader* GetShaderByRefName(const std::string& refName);
    //static Shader* GetShaderByRefName(const char* refName);
    static bool ReloadAllShaders();

  protected:
    bool FinishProgram(const char* refName);
    void AddToShaderHash();
    void RemoveFromShaderHash();

  public:
    static bool RunTests();
  private:
    static int s_test_shader_refs; // Just a silly var for test cases, will probably remove soon.

  };
}

