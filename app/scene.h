#pragma once

#include <vector>
#include "../common/chunkloader.h"
#include "../common/component.h"
#include "../common/fourmath.h"

namespace fd {

class Camera;
class Entity;
class MeshBuffer;
class Mesh;
class Physics;
class QuaxolChunk;
class Shader;
class Texture;
class Render;

class Scene {
protected:
  typedef std::vector<Entity*> TEntityList;
  TEntityList m_dynamicEntities;
  TEntityList m_toBeDeleted;

  typedef std::vector<Vec4f> ColorList;
  ColorList m_colorArray;

  Mesh* m_pGroundPlane; // owned
  Shader* m_pGroundShader;

  ComponentBus m_componentBus;

public:
  // leaving some vars public to avoid getter/setter as an experiment
  // requires knowing about write update functions
  Physics* m_pPhysics; //owned

  TVecQuaxol m_quaxols;
  // shaders should not be here really, turning this class into dumping grounds
  Shader* m_pQuaxolShader; //not owned
  Mesh* m_pQuaxolMesh; //not owned
  Texture* m_pQuaxolAtlas;

  typedef std::vector<Texture*> TTextureList;
  TTextureList m_texList;
  MeshBuffer* m_pQuaxolBuffer; // owned

  QuaxolChunk* m_pQuaxolChunk;

public:
  Scene();
  ~Scene();

  bool Initialize();
  //void AddLoadedChunk(const ChunkLoader* pChunk);
  void TakeLoadedChunk(QuaxolChunk* pChunk);

  void SetQuaxolAt(const QuaxolSpec& pos, bool present);
  void SetQuaxolAt(const QuaxolSpec& pos, bool present, int type);
  void RenderQuaxols(Camera* pCamera, Shader* pShader);
  void RenderQuaxolChunk(Camera* pCamera, Shader* pShader);
  void RenderQuaxolsIndividually(Camera* pCamera, Shader* pShader); // deprecated

  // ugh this is all wrong, not going to be shader sorted, etc
  // but let's just do the stupid thing first
  void RenderEverything(Camera* pCamera, Render* pRender);
  void Step(float fDelta);
  
  // Let the scene do the allocation to allow for mem opt
  // Some kind of entity def that at least includes shader, texture and mesh
  // types would be better for this as we could pre-sort by shader/texture/mesh
  Entity* AddEntity();
  void RemoveEntity(Entity* pEntity);
  void OnDeleteEntity(Entity* pEntity);
  void RenderDynamicEntities(Camera* pCamera, Render* pRender);

  // trying some stuff for stencil portals, this gets called if a scene is a dependent render
  // from it's own callgraph. 
  Scene* RecursiveClone();

  // Hacky garbage, should be on the mesh/quaxol
  void BuildColorArray();
  void AddTexture(Texture* pTex);

  void RenderGroundPlane(Camera* pCamera);

  ComponentBus& GetComponentBus() { return m_componentBus; }

protected:
  // horrible way to index textures
  // going to need a shader context or something soon
  void SetTexture(int index, int hTex);

};

} // namespace fd