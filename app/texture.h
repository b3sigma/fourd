#pragma once

#include <GL/glew.h>
#include <vector>

namespace fd {
  class Texture {
  protected:
    GLuint texture_id_;
    GLsizei width_;
    GLsizei height_;
    GLenum format_;
    GLenum internal_format_;

  public:
    Texture() : texture_id_(0) {}
    ~Texture();

    bool LoadFromFile(const char* filename);
    void Release();

    GLuint GetTextureID() const { return texture_id_; }

  };
  typedef std::vector<Texture*> TTextureList;

}; // namespace fd
