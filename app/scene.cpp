
#include <GL/glew.h>

#include "scene.h"

#include "../common/camera.h"
#include "../common/mesh.h"
#include "../common/physics.h"
#include "../common/quaxol.h"
#include "../common/components/physics_component.h"

#include "entity.h"
#include "glhelper.h"
#include "meshbuffer.h"
#include "shader.h"
#include "texture.h"

namespace fd {

Scene::Scene() 
  : m_pQuaxolShader(NULL)
  , m_pQuaxolMesh(NULL)
  , m_pQuaxolBuffer(NULL)
  , m_pQuaxolChunk(NULL)
{
  BuildColorArray();

  m_pPhysics = new Physics();

  m_componentBus.RegisterSignal(std::string("EntityDeleted"), this, &Scene::RemoveEntity);  // notification from entity that it is being deleted
  m_componentBus.RegisterSignal(std::string("DeleteEntity"), this, &Scene::OnDeleteEntity); // command from entity to delete it
}

Scene::~Scene() {
  delete m_pQuaxolBuffer;
  delete m_pQuaxolChunk;
  delete m_pPhysics;
}

void Scene::AddLoadedChunk(const ChunkLoader* pChunk) {
  m_quaxols.assign(pChunk->quaxols_.begin(), pChunk->quaxols_.end());

  QuaxolSpec offset(0, 0, 0, 0);
  Vec4f position(0, 0, 0, 0); // need to fill these out with appropriate values
  Vec4f blockSize(10.0f, 10.0f, 10.0f, 10.0f);
  m_pQuaxolChunk = new QuaxolChunk(position, blockSize);
  m_pQuaxolChunk->LoadFromList(&m_quaxols, &offset);
  m_pPhysics->AddChunk(m_pQuaxolChunk);
}

void Scene::AddTexture(Texture* pTex) {
  m_texList.push_back(pTex);
}

void Scene::SetTexture(int index, GLint hTex) {
  assert(index >= 0 && index < (int)m_texList.size());

  if(m_texList.empty())
    return;

  Texture* pTex = m_texList[index];
  glActiveTexture(GL_TEXTURE0);
  WasGLErrorPlusPrint();
  glBindTexture(GL_TEXTURE_2D, pTex->GetTextureID());
  WasGLErrorPlusPrint();
  glUniform1i(hTex, 0);
  WasGLErrorPlusPrint();
}

// Let the scene do the allocation to allow for mem opt
Entity* Scene::AddEntity() {
  // the less stupid way to do this is batches
  Entity* pEntity = new Entity();
  m_dynamicEntities.push_back(pEntity);
  m_componentBus.AddComponent((Component*)pEntity);
  return pEntity;
}

void Scene::RemoveEntity(Entity* pEntity) {
  m_dynamicEntities.erase(
      std::remove(m_dynamicEntities.begin(), m_dynamicEntities.end(), pEntity), 
      m_dynamicEntities.end());
}

void Scene::OnDeleteEntity(Entity* pEntity) {
  m_toBeDeleted.push_back(pEntity);  
}

void Scene::Step(float fDelta) {
  m_componentBus.Step(fDelta);

  for(auto pEntity : m_toBeDeleted) {
    delete pEntity;
  }
  m_toBeDeleted.resize(0);
}

// ugh this is all wrong, not going to be shader sorted, etc
// but let's just do the stupid thing first
// Also, shouldn't this be in a render class instead of scene?
void Scene::RenderEntitiesStupidly(Camera* pCamera) {
  for(const auto pEntity : m_dynamicEntities) {
    //TODO: cache shader transitions? or does opengl kind of handle it
    // as long as they are sorted?
    Shader* pShader = pEntity->m_pShader;
    if(pShader == NULL) continue; // should go away when we sort
    Mesh* pMesh = pEntity->m_pMesh;
    if(pMesh == NULL) continue; // should go away when we sort

    pShader->StartUsing();
    pShader->SetPosition(&(pEntity->m_position));
    pShader->SetOrientation(&(pEntity->m_orientation));
    pShader->SetCameraParams(pCamera);

    // this is getting ugly to look at in a hurry
    // need to do buffers soon
    int numTris = pMesh->getNumberTriangles();
    int startTriangle = 0;
    int endTriangle = numTris;
    glBegin(GL_TRIANGLES);
    Vec4f a, b, c;
    //int colorIndex = 0;
    for (int t = startTriangle; t < endTriangle && t < numTris; t++) {
      //glVertexAttrib4fv(hColor, colorArray[colorIndex].raw());
      pMesh->getTriangle(t, a, b, c);
      glVertex4fv(a.raw());
      glVertex4fv(b.raw());
      glVertex4fv(c.raw());
      //if ((t+1) % 2 == 0) {
      //  colorIndex = (colorIndex + 1) % colorArray.size();
      //}
    }
    glEnd();
    pShader->StopUsing();
  }

  if(!m_pQuaxolShader || !m_pQuaxolMesh)
    return;



  // if you thought the above was ugly and wasteful, just wait!
  WasGLErrorPlusPrint();
  m_pQuaxolShader->StartUsing();
  m_pQuaxolShader->SetCameraParams(pCamera);
  WasGLErrorPlusPrint();
  Mat4f worldMatrix;
  worldMatrix.storeIdentity();
  m_pQuaxolShader->SetOrientation(&worldMatrix);
  WasGLErrorPlusPrint();

  GLint hTex0 = m_pQuaxolShader->getUniform("texDiffuse0");
  WasGLErrorPlusPrint();
  if (hTex0 != -1) {
    SetTexture(0, hTex0);
  }

  static bool renderChunk = true;
  if(renderChunk) {
    m_pQuaxolShader->SetPosition(&Vec4f(0,0,0,0));
    GLuint colorHandle = m_pQuaxolShader->GetColorHandle();

    glBegin(GL_TRIANGLES);

    IndexList& indices = m_pQuaxolChunk->m_indices; 
    VertList& verts = m_pQuaxolChunk->m_verts;
    int numTris = (int)indices.size() / 3;
    int currentIndex = 0;
    for(int tri = 0; tri < numTris; ++tri) {

      const Vec4f& a = verts[indices[currentIndex++]]; 
      const Vec4f& b = verts[indices[currentIndex++]]; 
      const Vec4f& c = verts[indices[currentIndex++]]; 

      if(colorHandle != -1) {
        float minW = min(a.w, min(b.w, c.w));
        int colorIndex = ((int)abs(minW)) % m_colorArray.size();
        glVertexAttrib4fv(colorHandle,
            m_colorArray[colorIndex].raw());
      }

      glVertex4fv(a.raw());
      glVertex4fv(b.raw());
      glVertex4fv(c.raw());
    }
    glEnd();

  } else {
    for (auto quaxol : m_quaxols) {
    
      const QuaxolSpec& q = quaxol;

      float shift_amount = 10.0f;
      Vec4f shift;
      shift.x = shift_amount * q.x;
      shift.y = shift_amount * q.y;
      shift.z = shift_amount * q.z;
      shift.w = shift_amount * q.w;
      m_pQuaxolShader->SetPosition(&shift);
      WasGLErrorPlusPrint();

      if (m_texList.size() > 0) {
        int layerTexIndex = abs(q.w) % m_texList.size();
        SetTexture(layerTexIndex, hTex0);
      }

      int tesseractTris = m_pQuaxolMesh->getNumberTriangles();
      int startTriangle = 0;
      int endTriangle = tesseractTris;
      WasGLErrorPlusPrint();

      GLuint colorHandle = m_pQuaxolShader->GetColorHandle();
      glBegin(GL_TRIANGLES);
      //WasGLErrorPlusPrint();
      Vec4f a, b, c;
      int colorIndex = abs(q.w) % m_colorArray.size();
      for (int t = startTriangle; t < endTriangle && t < tesseractTris; t++) {
        if(colorHandle != -1) {
          glVertexAttrib4fv(colorHandle,
              m_colorArray[colorIndex].raw());
        }
        //WasGLErrorPlusPrint();
        m_pQuaxolMesh->getTriangle(t, a, b, c);
        glVertex4fv(a.raw());
        glVertex4fv(b.raw());
        glVertex4fv(c.raw());
        //WasGLErrorPlusPrint();
        //if ((t+1) % 2 == 0) {
        //  colorIndex = (colorIndex + 1) % m_colorArray.size();
        //}
      }
      //WasGLErrorPlusPrint();
      glEnd();
      WasGLErrorPlusPrint();
    }
  }

  m_pQuaxolShader->StopUsing();
  WasGLErrorPlusPrint();
}

void Scene::BuildColorArray() {
  int numSteps = 8;
  m_colorArray.reserve(numSteps);
  for (int steps = 0; steps < numSteps; steps++) {
    m_colorArray.push_back(Vec4f(
      (float)((steps + 2) % 3) / 2.0f, 
      (float)(steps + 1) / (float)numSteps, 
      (float)((steps + 3) % 5) / 4.0f,
      1));
  }
}

} // namespace fd
