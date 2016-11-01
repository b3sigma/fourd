#include <memory>
#include "../common/mesh.h"
#include "../common/components/animated_rotation.h"
#include "entity.h"
#include "render_helper.h"
#include "render.h"
#include "scene.h"

extern fd::Scene g_scene; // fourd.cpp
extern fd::Render g_renderer; // fourd.cpp

namespace fd {

typedef std::list<std::unique_ptr<Mesh>> MeshList;
MeshList g_eyeCandyMeshes;

void RenderHelper::AddEyeCandy(EyeCandyTypes type, const Vec4f& pos) {
  g_eyeCandyMeshes.emplace_back(new Mesh());
  Mesh* candy = g_eyeCandyMeshes.back().get();
  const float size = 30.0f;
  const float smallSize = 15.0f;
  Vec4f smallOff(13.0f, 0.0f, 0.0f, 3.0f);
  switch(type) {
    case EyeCandyQuad:
      candy->buildQuad(smallSize, smallOff, Vec4f());
      break;
    case EyeCandyCube:
      candy->buildCube(smallSize, smallOff, Vec4f());
      break;
    case EyeCandyTesseract:
      candy->buildTesseract(smallSize, smallOff, Vec4f());
      break;
    case EyeCandy16Cell:
      candy->build16cell(size, Vec4f());
      break;
    case EyeCandy24Cell: {
      const float shift = 8.5f;
      const Vec4f offsetSize(shift, shift, 0.5f, shift * 2);
      candy->buildCaylay24Cell(size, offsetSize);
    } break;
    case EyeCandy120Cell: {
      const float shift = 5.5f;
      const Vec4f offsetSize(size / 2 + shift, size / 2 + shift, shift, size / 2 + shift);
      candy->buildCaylay120Cell(size, offsetSize);
    } break;
    case EyeCandy600Cell: {
      const float shift = 5.5f;
      const Vec4f offsetSize(size / 2 + shift, size / 2 + shift, shift, size / 2 + shift);
      candy->buildCaylay600Cell(size, offsetSize);
    } break;
  }

  Entity* pEntity = g_renderer.GetFirstScene()->AddEntity();
  // ugh need like a mesh manager and better approach to shader handling
  //pEntity->Initialize(candy, LoadShader("ColorBlend"), NULL);
  pEntity->Initialize(candy, g_renderer.LoadShader("ColorBlendClipped"), NULL);
  pEntity->m_position = pos;
  pEntity->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI * 2.0f, Camera::RIGHT, Camera::INSIDE,
      -20.0f, true));
}

void RenderHelper::RenderTess(Vec4f pos, const Mat4f* rotation) {
  g_eyeCandyMeshes.emplace_back(new Mesh());
  Mesh* candy = g_eyeCandyMeshes.back().get();
  const float size = 30.0f;
  const float smallSize = 15.0f;
  Vec4f smallOff(13.0f, 0.0f, 0.0f, 3.0f);
  candy->buildTesseract(smallSize, smallOff, Vec4f());
  
  Entity* pEntity = g_renderer.GetFirstScene()->AddEntity();
  // ugh need like a mesh manager and better approach to shader handling
  //pEntity->Initialize(candy, LoadShader("ColorBlend"), NULL);
  pEntity->Initialize(candy, g_renderer.LoadShader("ColorBlendClipped"), NULL);
  pEntity->m_position = pos;
  if(rotation) {
    pEntity->m_orientation = *rotation;
  }
  pEntity->GetComponentBus().AddComponent(
      new AnimatedRotation((float)PI * 2.0f, Camera::RIGHT, Camera::INSIDE,
      -20.0f, true));
}

} // namespace fd 