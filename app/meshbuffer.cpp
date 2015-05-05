#include <GL/glew.h>
#include "meshbuffer.h"

#include "../common/fourmath.h"
#include "../common/mesh.h"
#include "glhelper.h"

namespace fd {

MeshBuffer::MeshBuffer() : m_vertsId(-1), m_indicesId(-1),
      m_primitiveType(0), m_count(0), m_pMesh(NULL) {}

MeshBuffer::~MeshBuffer() {
  
}

bool MeshBuffer::LoadFromMesh(Mesh* pMesh) {
  m_pMesh = pMesh;

  if (m_pMesh->getNumberTriangles() <= 0)
    return false; //or maybe true?

  m_count = m_pMesh->getNumberTriangles();
  m_primitiveType = GL_TRIANGLES;

  glGenBuffers(1, &m_vertsId);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertsId);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4f) * m_pMesh->_verts.size(),
      &(m_pMesh->_verts[0]), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &m_indicesId);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesId);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
      sizeof(m_pMesh->_indices[0]) * m_pMesh->_indices.size(),
      &(m_pMesh->_indices[0]), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glGenVertexArrays(1, &m_vertAttribuesId);
  //glBindVertexArray(m_vertAttribuesId);
  //glBindVertexArray(0);

  return true;
}

bool MeshBuffer::Draw(GLuint vertPositionId) {

  glBindVertexArray(m_vertsId);
  glEnableVertexAttribArray(vertPositionId);
  glVertexAttribPointer(vertPositionId, sizeof(Vec4f), GL_FLOAT, GL_FALSE, 
    0 /* stride */, 0 /* offset pointer */);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesId);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertsId);

  glDrawArrays(m_primitiveType, 0 /* first */, m_count);
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(vertPositionId);

  return WasGLErrorPlusPrint();
}

} // namespace fd