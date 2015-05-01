#pragma once

#include "../common/fourmath.h"
#include "../common/component.h"

namespace fd {

class Mesh;
class Shader;

class Entity {
protected:
  Mat4f _orientation;
  Vec4f _position;

  ComponentBus _componentBus;
  Mesh* _pMesh; // not owned
  Shader* _pShader; // not owned

public:
  Entity();

  bool Initialize(Mesh* pMesh, Shader* pShader);

  ComponentBus& GetComponentBus() { return _componentBus; }

  void* operator new(size_t size) { return _aligned_malloc(size, 16); }
  void operator delete(void* p) { _aligned_free(p); }
};

}; // namespace fd