
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
#include "render.h"
#include "shader.h"
#include "texture.h"

namespace fd {

Scene::Scene() 
  : m_pQuaxolShader(NULL)
  , m_pQuaxolMesh(NULL)
  , m_pQuaxolBuffer(NULL)
  , m_pQuaxolChunk(NULL)
  , m_pQuaxolAtlas(NULL)
  , m_pGroundPlane(NULL)
{
  BuildColorArray();

  m_pPhysics = new Physics();

  m_componentBus.RegisterSignal(std::string("EntityDeleted"), this,
      &Scene::RemoveEntity);  // notification from entity that it is being deleted
  m_componentBus.RegisterSignal(std::string("DeleteEntity"), this,
      &Scene::OnDeleteEntity); // command from entity to delete it
}

Scene::~Scene() {
  delete m_pQuaxolBuffer;
  delete m_pQuaxolChunk;
  delete m_pPhysics;
  delete m_pGroundPlane;
}

bool Scene::Initialize() {
  m_pGroundPlane = new Mesh;
  float extents = 1000.0f;
  Vec4f minGround(-extents, -extents, -extents, -extents);
  Vec4f maxGround(extents, extents, 0.0f, extents);
  m_pGroundPlane->buildTesseract(minGround, maxGround);

  m_pGroundShader = new Shader();
  m_pGroundShader->AddDynamicMeshCommonSubShaders();
  if(!m_pGroundShader->LoadFromFileDerivedNames("Ground")) {
    return false;
  }

  m_pQuaxolAtlas = new Texture();
  if(!m_pQuaxolAtlas->LoadFromFile("data\\textures\\atlas.png")) {
    return false;
  }

  m_pQuaxolShader = new Shader();
  m_pQuaxolShader->AddDynamicMeshCommonSubShaders();
  if(!m_pQuaxolShader->LoadFromFileDerivedNames("Rainbow")) {
  //if(!m_pQuaxolShader->LoadFromFileDerivedNames("ColorBlendClipped")) {
    return false;
  }
 
  return true;
}

void Scene::TakeLoadedChunk(QuaxolChunk* pChunk) {
  m_pQuaxolChunk = pChunk;
  m_pPhysics->AddChunk(m_pQuaxolChunk);
}

//void Scene::AddLoadedChunk(const ChunkLoader* pChunk) {
//  m_quaxols.assign(pChunk->quaxols_.begin(), pChunk->quaxols_.end());
//
//  QuaxolSpec offset(0, 0, 0, 0);
//  Vec4f position(0, 0, 0, 0); // need to fill these out with appropriate values
//  Vec4f blockSize(10.0f, 10.0f, 10.0f, 10.0f);
//  m_pQuaxolChunk = new QuaxolChunk(position, blockSize);
//  m_pQuaxolChunk->LoadFromList(&m_quaxols, &offset);
//  m_pPhysics->AddChunk(m_pQuaxolChunk);
//}

void Scene::SetQuaxolAt(const QuaxolSpec& pos, bool present) {
  if(!m_pQuaxolChunk) return;
  m_pQuaxolChunk->SetAt(pos, present);
  m_pQuaxolChunk->UpdateRendering();
}

void Scene::SetQuaxolAt(const QuaxolSpec& pos, bool present, int type) {
  if(!m_pQuaxolChunk) return;
  m_pQuaxolChunk->SetAt(pos, present, type);
  m_pQuaxolChunk->UpdateRendering();
}

void Scene::AddTexture(Texture* pTex) {
  m_texList.push_back(pTex);
}

// how does this work without the glUniform1i? Fragile! Except it errored once so I removed it.
void Scene::SetTexture(int index, int hTex) {
  assert(index >= 0 && index < (int)m_texList.size());

  if(m_texList.empty())
    return;

  Texture* pTex = m_texList[index];
  glActiveTexture(GL_TEXTURE0);
  WasGLErrorPlusPrint();
  glBindTexture(GL_TEXTURE_2D, pTex->GetTextureID());
  WasGLErrorPlusPrint();
  //glUniform1i(hTex, 0);
  //WasGLErrorPlusPrint();
}

// Let the scene do the allocation to allow for mem opt
Entity* Scene::AddEntity() {
  // the less stupid way to do this is batches
  Entity* pEntity = new Entity(this);
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

bool RenderMesh(Camera* pCamera, Shader* pShader, Mesh* pMesh,
    const Vec4f& position, const Mat4f& orientation) {
  if(pShader == NULL) return false; // should go away when we sort
  if(pMesh == NULL) return false; // should go away when we sort

  pShader->StartUsing();
  pShader->SetPosition(&position);
  pShader->SetOrientation(&orientation);
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

  return true;
}

void Scene::RenderGroundPlane(Camera* pCamera) {
  static bool renderGroundPlane = false;
  if(!renderGroundPlane)
    return;

  static Mat4f groundOrientation = Mat4f().storeIdentity();
  static Vec4f groundPosition = Vec4f().storeZero();
  RenderMesh(pCamera, m_pGroundShader, m_pGroundPlane,
      groundPosition, groundOrientation);
}

// ugh this is all wrong, not going to be shader sorted, etc
// but let's just do the stupid thing first
// Also, shouldn't this be in a render class instead of scene?
// now both actually
void Scene::RenderEverything(Camera* pCamera) {
  RenderGroundPlane(pCamera);

  RenderDynamicEntities(pCamera);

  RenderQuaxols(pCamera, m_pQuaxolShader);
}

// TODO: if this gets used more, will probably need split between alpha/non
void Scene::RenderDynamicEntities(Camera* pCamera) {
  for(const auto pEntity : m_dynamicEntities) {
    //TODO: sort by shader transition
    RenderMesh(pCamera, pEntity->m_pShader, pEntity->m_pMesh,
        pEntity->m_position, pEntity->m_orientation);
    pEntity->GetComponentBus().SendSignal("AfterRender", SignalN<Camera*>(), pCamera);
  }
}

void Scene::RenderQuaxolChunk(Camera* pCamera, Shader* pShader) {
  if(!m_pQuaxolChunk) return;

  WasGLErrorPlusPrint();

  pShader->StartUsing();
  pShader->SetCameraParams(pCamera);
  Mat4f worldMatrix;
  worldMatrix.storeIdentity();
  pShader->SetOrientation(&worldMatrix);
  //pShader->SetOrientation(

  WasGLErrorPlusPrint();

  if (m_pQuaxolAtlas) {
    GLint hTex0 = pShader->getUniform("texDiffuse0");
    if (hTex0 != -1) {
      glActiveTexture(GL_TEXTURE0);
      WasGLErrorPlusPrint();
      glBindTexture(GL_TEXTURE_2D, m_pQuaxolAtlas->GetTextureID());
  //WasGLErrorPlusPrint();
  //    glUniform1i(hTex0, 0);
    }
  }

  GLint hTexCoord = glGetAttribLocation(pShader->getProgramId(), "vertCoord");
  //GLint hTexCoord = pShader->getAttrib("vertCoord");

  pShader->SetPosition(&Vec4f(0, 0, 0, 0));
  GLuint colorHandle = pShader->GetColorHandle();

  glBegin(GL_TRIANGLES);

  // TODO: use vbo, as we have already done the work to put things into
  // a nice format.
  IndexList& indices = m_pQuaxolChunk->m_indices;
  VecList& verts = m_pQuaxolChunk->m_verts;
  QVertList& packVerts = m_pQuaxolChunk->m_packVerts;
  int numTris = (int)indices.size() / 3;
  int currentIndex = 0;
  for (int tri = 0; tri < numTris; ++tri) {

    const QuaxolVert& vA = packVerts[indices[currentIndex]];
    const Vec4f& a = verts[indices[currentIndex++]];
    const QuaxolVert& vB = packVerts[indices[currentIndex]];
    const Vec4f& b = verts[indices[currentIndex++]];
    const QuaxolVert& vC = packVerts[indices[currentIndex]];
    const Vec4f& c = verts[indices[currentIndex++]];

    if (colorHandle != -1) {
      float minW = min(a.w, min(b.w, c.w));
      int colorIndex = ((int)abs(minW)) % m_colorArray.size();
      glVertexAttrib4fv(colorHandle, m_colorArray[colorIndex].raw());
    }

    if (hTexCoord != -1) {
      const float invTexSteps = 1.0f / 16.0f;
      int firstTriOffset = tri % 2; 
      glVertexAttrib2f(hTexCoord, 
          (float)((vA._uvInd % 8) + 0) * invTexSteps, 
          (float)((vA._uvInd / 8) + firstTriOffset) * invTexSteps);
      glVertex4fv(a.raw());
      glVertexAttrib2f(hTexCoord, 
          (float)((vB._uvInd % 8) + 1) * invTexSteps, 
          (float)((vB._uvInd / 8) + 0) * invTexSteps);
      glVertex4fv(b.raw());
      glVertexAttrib2f(hTexCoord, 
          (float)((vC._uvInd % 8) + firstTriOffset) * invTexSteps, 
          (float)((vC._uvInd / 8) + 1) * invTexSteps);
      glVertex4fv(c.raw());
    } else {
      glVertex4fv(a.raw());
      glVertex4fv(b.raw());
      glVertex4fv(c.raw());
    }
  }
  glEnd();

  pShader->StopUsing();
  
}

void Scene::RenderQuaxolsIndividually(Camera* pCamera, Shader* pShader) {
  if(!m_pQuaxolMesh) return;

  WasGLErrorPlusPrint();
  pShader->StartUsing();
  pShader->SetCameraParams(pCamera);
  WasGLErrorPlusPrint();
  Mat4f worldMatrix;
  worldMatrix.storeIdentity();
  pShader->SetOrientation(&worldMatrix);
  WasGLErrorPlusPrint();

  GLint hTex0 = pShader->getUniform("texDiffuse0");
  WasGLErrorPlusPrint();
  if (hTex0 != -1) {
    SetTexture(0, hTex0);
  }

  for (auto quaxol : m_quaxols) {  
    const QuaxolSpec& q = quaxol;

    float shift_amount = 10.0f;
    Vec4f shift;
    shift.x = shift_amount * q.x;
    shift.y = shift_amount * q.y;
    shift.z = shift_amount * q.z;
    shift.w = shift_amount * q.w;
    pShader->SetPosition(&shift);
    WasGLErrorPlusPrint();

    if (m_texList.size() > 0) {
      int layerTexIndex = abs(q.w) % m_texList.size();
      SetTexture(layerTexIndex, hTex0);
    }

    int tesseractTris = m_pQuaxolMesh->getNumberTriangles();
    int startTriangle = 0;
    int endTriangle = tesseractTris;
    WasGLErrorPlusPrint();

    GLuint colorHandle = pShader->GetColorHandle();
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

  pShader->StopUsing();
}

void Scene::RenderQuaxols(Camera* pCamera, Shader* pShader) {
  if(!pShader)
    return;

  static bool renderChunk = true; // vs blocks individually
  if(renderChunk) {
    RenderQuaxolChunk(pCamera, pShader);
  } else {
    RenderQuaxolsIndividually(pCamera, pShader);
  }
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
