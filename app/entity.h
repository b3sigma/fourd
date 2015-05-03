#pragma once

#include "../common/fourmath.h"
#include "../common/component.h"

namespace fd {

class Mesh;
class Shader;
class Texture;

class Entity : Component {
public:
  // leaving vars public to avoid getter/setter as an experiment
  // requires knowing about write update functions
  Mat4f m_orientation;
  Vec4f m_position;

  ComponentBus m_componentBus;
  Mesh* m_pMesh; // not owned
  Shader* m_pShader; // not owned
  typedef std::vector<Texture*> TTextureList;
  TTextureList m_textures;

public:
  Entity();
  ~Entity();

  virtual void OnConnected();

  void OnStepSignal(float delta);
  void OnDeleteSelf();

  bool Initialize(Mesh* pMesh, Shader* pShader, const TTextureList* textures);

  ComponentBus& GetComponentBus() { return m_componentBus; }

  void* operator new(size_t size) { return _aligned_malloc(size, 16); }
  void operator delete(void* p) { _aligned_free(p); }
};

}; // namespace fd