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
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, // level
      internal_format_, width_, height_, 0, // border
      format_, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return true;
  }

  void Texture::Release() {
    glDeleteTextures(1, &texture_id_);
  }

  Texture::~Texture() {
    Release();
  }

} // ns fd