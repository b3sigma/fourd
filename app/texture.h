#pragma once

#include <GL/glew.h>
#include <vector>
#include <set>
#include <string>

namespace fd {

  class Texture;
  typedef std::vector<Texture*> TTextureList;
  typedef std::set<Texture*> TTextureSet;

  class Texture {
  public:
    GLuint m_texture_id;
    GLuint m_framebuffer_id;
    GLsizei m_width;
    GLsizei m_height;
    GLenum m_format;
    GLenum m_internal_format;
    std::string m_name;

    static TTextureSet s_textureCache; //ugh;

  public:
    Texture() : m_texture_id(-1), m_framebuffer_id(-1) {}
    ~Texture();

    bool LoadFromFile(const char* filename);
    bool CreateRenderTarget(int sizeX, int sizeY);
    bool CreateDepthTarget(int sizeX, int sizeY);
    bool CreateFrameTarget(int sizeX, int sizeY); //yeahhh, I dunno
    void Release();

    GLuint GetTextureID() const { return m_texture_id; }

  public:
    static void DeinitializeTextureCache();

  protected:
    static void AddToTextureCache(Texture* pTexture);
  };

}; // namespace fd
