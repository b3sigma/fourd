#include "glhelper.h"
#include "texture.h"


#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

namespace fd {

  bool Texture::LoadFromFile(const char* filename) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);

    if (data == NULL) {
      return false;
    }

    width_ = static_cast<GLsizei>(width);
    height_ = static_cast<GLsizei>(height);

    format_ = 0;
    internal_format_ = 0;

    switch (channels) {
    case 1:
      format_ = GL_LUMINANCE;
      internal_format_ = format_;
      break;
    case 2:
      format_ = GL_LUMINANCE_ALPHA;
      internal_format_ = format_;
      break;
    case 3:
      format_ = GL_RGB;
      internal_format_ = GL_SRGB;
      break;
    case 4:
      format_ = GL_RGBA;
      internal_format_ = GL_SRGB_ALPHA;
      break;
    default:
      printf("Texture had unexpected number of channels: %d\n", channels);
      stbi_image_free(data);
      return false;
    }

    glGenTextures(1, &texture_id_);
    WasGLErrorPlusPrint();
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    WasGLErrorPlusPrint();
    // TODO: Check if img format that includes mips, like dxt
    bool generateMipmaps = true; 
    if (generateMipmaps) {
      if(!CheckGLUErr(gluBuild2DMipmaps(GL_TEXTURE_2D, channels, 
          width_, height_, format_, GL_UNSIGNED_BYTE, data))) {
        stbi_image_free(data);
        glDeleteTextures(1, &texture_id_);
        texture_id_ = -1;
        return false;
      }
    } else {
      glTexImage2D(GL_TEXTURE_2D, 0 /* level */,
          internal_format_, width_, height_, 0 /* border */,
          format_, GL_UNSIGNED_BYTE, data);
    }
    WasGLErrorPlusPrint();

    //glGenerateTextureMipmap(texture_id_);
    //WasGLErrorPlusPrint();

    glBindTexture(GL_TEXTURE_2D, 0);
    WasGLErrorPlusPrint();

    
    stbi_image_free(data);
    return true;
  }

  void Texture::Release() {
    if (texture_id_ >= 0) {
      glDeleteTextures(1, &texture_id_);
    }
  }

  Texture::~Texture() {
    Release();
  }

} // ns fd