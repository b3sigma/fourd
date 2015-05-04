#include <GL/glew.h>
#include "meshbuffer.h"

#include "../common/fourmath.h"
#include "../common/mesh.h"

namespace fd {

MeshBuffer::MeshBuffer() : m_vertsId(-1), m_indicesId(-1),
      m_primitiveType(0), m_count(0), m_pMesh(NULL) {}

MeshBuffer::~MeshBuffer() {
}

bool MeshBuffer::LoadFromMesh(Mesh* pMesh) {
  m_pMesh = pMesh;

  if (m_pMesh->getNumberTriangles() <= 0)
    return false; //or maybe true?

  glGenBuffers(1, &m_vertsId);
  glGenVertexArrays(1, &m_indicesId);

  glBindVertexArray(m_vertsId);
  glBindBuffer(GL_ARRAY_BUFFER, m_indicesId);

  glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4f) * m_pMesh->_verts.size(),
      &(m_pMesh->_verts[0]), GL_STATIC_DRAW);



  return true;

}

} // namespace fd