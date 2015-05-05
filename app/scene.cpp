
#include <GL/glew.h>

#include "scene.h"

#include "../common/camera.h"
#include "../common/mesh.h"

#include "entity.h"
#include "glhelper.h"
#include "meshbuffer.h"
#include "shader.h"
#include "texture.h"

namespace fd {

Scene::Scene() 
  : m_pQuaxolShader(NULL)
  , m_pQuaxolMesh(NULL)
  , m_pQuaxolTex(NULL)
  , m_pQuaxolBuffer(NULL)
{
  BuildColorArray();

  m_componentBus.RegisterSignal(std::string("EntityDeleted"), this, &Scene::RemoveEntity);  // notification from entity that it is being deleted
  m_componentBus.RegisterSignal(std::string("DeleteEntity"), this, &Scene::OnDeleteEntity); // command from entity to delete it
}

Scene::~Scene() {
  delete m_pQuaxolBuffer;
}

void Scene::AddCamera(Camera* pCamera) {
  m_cameras.push_back(pCamera);
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
  for(auto pCamera : m_cameras) {
    pCamera->Step(fDelta);
  }

  for(auto pEntity : m_toBeDeleted) {
    delete pEntity;
  }
  m_toBeDeleted.resize(0);
}

// ugh this is all wrong, not going to be shader sorted, etc
// but let's just do the stupid thing first
// Also, shouldn't this be in a render class instead of scene?
void Scene::RenderEntitiesStupidly() {

  for(const auto pCamera : m_cameras) {
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
      continue;

    // if you thought the above was ugly and wasteful, just wait!
    WasGLErrorPlusPrint();
    m_pQuaxolShader->StartUsing();
    m_pQuaxolShader->SetCameraParams(pCamera);
    WasGLErrorPlusPrint();
    Mat4f worldMatrix;
    worldMatrix.storeIdentity();
    m_pQuaxolShader->SetOrientation(&worldMatrix);
    WasGLErrorPlusPrint();
    if(m_pQuaxolTex) {
      GLint hTex0 = m_pQuaxolShader->getUniform("texDiffuse0");
      WasGLErrorPlusPrint();
      if (hTex0 != -1) {
        glActiveTexture(GL_TEXTURE0);
        WasGLErrorPlusPrint();
        glBindTexture(GL_TEXTURE_2D, m_pQuaxolTex->GetTextureID());
        WasGLErrorPlusPrint();
        glUniform1i(hTex0, 0);
        WasGLErrorPlusPrint();
      }
    }

    for (auto quaxol : m_quaxols) {
    
      const Quaxol& q = quaxol;

      float shift_amount = 10.0f;
      Vec4f shift;
      shift.x = shift_amount * q.x;
      shift.y = shift_amount * q.y;
      shift.z = shift_amount * q.z;
      shift.w = shift_amount * q.w;
      m_pQuaxolShader->SetPosition(&shift);
      WasGLErrorPlusPrint();

      int tesseractTris = m_pQuaxolMesh->getNumberTriangles();
      int startTriangle = 0;
      int endTriangle = tesseractTris;
      WasGLErrorPlusPrint();

      GLuint colorHandle = m_pQuaxolShader->GetColorHandle();
      glBegin(GL_TRIANGLES);
      //WasGLErrorPlusPrint();
      Vec4f a, b, c;
      int colorIndex = 0;
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
        if ((t+1) % 2 == 0) {
          colorIndex = (colorIndex + 1) % m_colorArray.size();
        }
      }
      //WasGLErrorPlusPrint();
      glEnd();
      WasGLErrorPlusPrint();
    }

    m_pQuaxolShader->StopUsing();
    WasGLErrorPlusPrint();
    
  }
}

void Scene::BuildColorArray() {
  int numSteps = 8;
  m_colorArray.reserve(numSteps);
  for (int steps = 0; steps < numSteps; steps++) {
    m_colorArray.push_back(Vec4f(0, (float)(steps + 1) / (float)numSteps, 0, 1));
  }
}

} // namespace fd
