#pragma once

#include <vector>
#include "../common/chunkloader.h"
#include "../common/component.h"
#include "../common/fourmath.h"

namespace fd {

class Camera;
class Entity;
class Shader;
class Texture;
class MeshBuffer;
class Mesh;

class Scene {
protected:
  typedef std::vector<Entity*> TEntityList;
  TEntityList m_dynamicEntities;
  TEntityList m_toBeDeleted;

  typedef std::vector<Camera*> TCameraList;
  TCameraList m_cameras; //not owned

  typedef std::vector<Vec4f> ColorList;
  ColorList m_colorArray;

  ComponentBus m_componentBus;

public:
  // leaving some vars public to avoid getter/setter as an experiment
  // requires knowing about write update functions
  TVecQuaxol m_quaxols;
  Shader* m_pQuaxolShader; //not owned
  Mesh* m_pQuaxolMesh; //not owned
  Texture* m_pQuaxolTex; //not owned
  MeshBuffer* m_pQuaxolBuffer; // owned

public:
  Scene();
  ~Scene();


  // should only be one or two of these any style is fine
  // but this is inconsistent with the entity thing...
  void AddCamera(Camera* pCamera);

  // Let the scene do the allocation to allow for mem opt
  // Some kind of entity def that at least includes shader, texture and mesh
  // types would be better for this as we could pre-sort by shader/texture/mesh
  Entity* AddEntity();
  void RemoveEntity(Entity* pEntity);

  void OnDeleteEntity(Entity* pEntity);

  void Step(float fDelta);

  // ugh this is all wrong, not going to be shader sorted, etc
  // but let's just do the stupid thing first
  void RenderEntitiesStupidly();

  // Hacky garbage, should be on the mesh/quaxol
  void BuildColorArray();
};

} // namespace fd