#include "glhelper.h"
#include "texture.h"
#include "../common/fourmath.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

namespace fd {

  TTextureSet Texture::s_textureCache;

  void Texture::DeinitializeTextureCache() {
    for (auto pTex : s_textureCache) {
      delete pTex;
    }
    s_textureCache.clear();
  }

  void Texture::AddToTextureCache(Texture* pTexture) {
    // as it's pointer based, should be idempotent
    s_textureCache.insert(pTexture); 
  }

  bool Texture::LoadFromFile(const char* filename) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);

    if (data == NULL) {
      return false;
    }

    m_width = static_cast<GLsizei>(width);
    m_height = static_cast<GLsizei>(height);

    m_format = 0;
    m_internal_format = 0;

    switch (channels) {
    case 1:
      m_format = GL_LUMINANCE;
      m_internal_format = m_format;
      break;
    case 2:
      m_format = GL_LUMINANCE_ALPHA;
      m_internal_format = m_format;
      break;
    case 3:
      m_format = GL_RGB;
      m_internal_format = GL_SRGB;
      break;
    case 4:
      m_format = GL_RGBA;
      m_internal_format = GL_SRGB_ALPHA;
      break;
    default:
      printf("Texture had unexpected number of channels: %d\n", channels);
      stbi_image_free(data);
      return false;
    }

    glGenTextures(1, &m_texture_id);
    WasGLErrorPlusPrint();
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    WasGLErrorPlusPrint();
    // TODO: Check if img format that includes mips, like dxt
    bool generateMipmaps = true; 
    if (generateMipmaps) {
      if(!WasGLUErr(gluBuild2DMipmaps(GL_TEXTURE_2D, channels, 
          m_width, m_height, m_format, GL_UNSIGNED_BYTE, data))) {
        stbi_image_free(data);
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = -1;
        return false;
      }
    } else {
      glTexImage2D(GL_TEXTURE_2D, 0 /* level */,
          m_internal_format, m_width, m_height, 0 /* border */,
          m_format, GL_UNSIGNED_BYTE, data);
    }
    WasGLErrorPlusPrint();

    //glGenerateTextureMipmap(texture_id_);
    //WasGLErrorPlusPrint();

    glBindTexture(GL_TEXTURE_2D, 0);
    WasGLErrorPlusPrint();

    
    stbi_image_free(data);
    
    AddToTextureCache(this);
    return true;
  }

  bool Texture::CreateRenderTarget(int sizeX, int sizeY) {
    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_width = sizeX;
    m_height = sizeY;
    m_format = GL_RGBA;
    m_internal_format = GL_SRGB_ALPHA;

    glTexImage2D(GL_TEXTURE_2D, 0 /* level */,
          m_internal_format, m_width, m_height, 0 /* border */,
          m_format, GL_UNSIGNED_BYTE, NULL /*data*/);
    glGenFramebuffers(1, &m_framebuffer_id);

    return !WasGLErrorPlusPrint();
  }

  bool Texture::CreateDepthTarget(int sizeX, int sizeY) {
    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_width = sizeX;
    m_height = sizeY;
    m_format = GL_DEPTH_COMPONENT;
    m_internal_format = GL_DEPTH_COMPONENT24;
   
    glTexImage2D(GL_TEXTURE_2D, 0 /* level */,
          m_internal_format, m_width, m_height, 0 /* border */,
          m_format, GL_UNSIGNED_BYTE, NULL /*data*/);
    glGenFramebuffers(1, &m_framebuffer_id);

    return !WasGLErrorPlusPrint();
  }


  void Texture::Release() {
    if (m_texture_id >= 0) {
      glDeleteTextures(1, &m_texture_id);
    }
    if (m_framebuffer_id >= 0) {
      glDeleteFramebuffers(1, &m_framebuffer_id);
    }
  }

  Texture::~Texture() {
    Release();
  }

} // ns fd