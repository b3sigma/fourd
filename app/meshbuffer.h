#pragma once

#include <GL/glew.h>

namespace fd {

class Mesh;
class MeshBuffer {
public:
  GLuint m_vertsId;
  GLuint m_vertAttribuesId;
  GLuint m_indicesId;
  GLenum m_primitiveType;
  GLint  m_count;

  Mesh*  m_pMesh;

public:
  MeshBuffer();
  ~MeshBuffer();

  bool LoadFromMesh(Mesh* pMesh);
  bool Draw(GLuint vertPositionId);

};

} // namespace fd
