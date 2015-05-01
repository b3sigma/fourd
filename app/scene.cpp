
#include <gl/glew.h>

#include "../common/camera.h"
#include "../common/mesh.h"
#include "scene.h"
#include "entity.h"
#include "shader.h"

namespace fd {

Scene::Scene() {
  BuildColorArray();
}

void Scene::AddCamera(Camera* pCamera) {
  m_cameras.push_back(pCamera);
}

// Let the scene do the allocation to allow for mem opt
Entity* Scene::AddEntity() {
  // the less stupid way to do this is batches
  Entity* pEntity = new Entity();
  m_dynamicEntities.push_back(pEntity);
  return pEntity;
}

void Scene::RemoveEntity(Entity* pEntity) {
  m_dynamicEntities.erase(
      std::remove(m_dynamicEntities.begin(), m_dynamicEntities.end(), pEntity), 
      m_dynamicEntities.end());
}

// ugh this is all wrong, not going to be shader sorted, etc
// but let's just do the stupid thing first
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

    // if you thought the above was ugly and wasteful, just wait!
    m_pQuaxolShader->StartUsing();
    m_pQuaxolShader->SetCameraParams(pCamera);
    Mat4f worldMatrix;
    worldMatrix.storeIdentity();
    m_pQuaxolShader->SetOrientation(&worldMatrix);

    for (auto quaxol : m_quaxols) {
    
      const Quaxol& q = quaxol;

      float shift_amount = 10.0f;
      Vec4f shift;
      shift.x = shift_amount * q.x;
      shift.y = shift_amount * q.y;
      shift.z = shift_amount * q.z;
      shift.w = shift_amount * q.w;
      m_pQuaxolShader->SetPosition(&shift);

      int tesseractTris = m_pQuaxolMesh->getNumberTriangles();
      int startTriangle = 0;
      int endTriangle = tesseractTris;
      glBegin(GL_TRIANGLES);
      Vec4f a, b, c;
      int colorIndex = 0;
      for (int t = startTriangle; t < endTriangle && t < tesseractTris; t++) {
        glVertexAttrib4fv(m_pQuaxolShader->GetColorHandle(),
            m_colorArray[colorIndex].raw());
        m_pQuaxolMesh->getTriangle(t, a, b, c);
        glVertex4fv(a.raw());
        glVertex4fv(b.raw());
        glVertex4fv(c.raw());
        if ((t+1) % 2 == 0) {
          colorIndex = (colorIndex + 1) % m_colorArray.size();
        }
      }
      glEnd();
    }

    m_pQuaxolShader->StopUsing();
    
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
