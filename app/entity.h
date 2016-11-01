#pragma once

#include "../common/fourmath.h"
#include "../common/component.h"
#include "../common/mem_helpers.h"

namespace fd {

class Mesh;
//class MeshBuffer;
class Scene;
class Shader;
class Texture;

class Entity : Component {
public:
  // leaving vars public to avoid getter/setter as an experiment
  // requires knowing about write update functions
  Mat4f m_orientation;
  Vec4f m_position;

  ComponentBus m_componentBus;
  Scene* m_scene;

  // TODO: abstract this stuff into mesh component?
  //MeshBuffer* m_pMeshBuffer;
  Mesh* m_pMesh; // not owned
  Shader* m_pShader; // not owned
  typedef std::vector<Texture*> TTextureList;
  TTextureList m_textures;

public:
  Entity(Scene* scene);
  ~Entity();

  virtual void OnConnected();

  void OnStepSignal(float delta);
  void OnDeleteSelf();

  bool Initialize(Mesh* pMesh, Shader* pShader, const TTextureList* textures);

  ComponentBus& GetComponentBus() { return m_componentBus; }

  ALIGNED_ALLOC_NEW_DEL_OVERRIDE
};

}; // namespace fd
