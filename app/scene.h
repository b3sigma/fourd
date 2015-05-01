#pragma once

#include <vector>
#include "../common/component.h"

namespace fd {

class Entity;
class Scene {
protected:
  typedef std::vector<Entity*> TEntityList;
  TEntityList m_dynamicEntities;

public:
  Scene() {}

  // Let the scene do the allocation to allow for mem opt
  // Some kind of entity def that at least includes shader, texture and mesh
  // types would be better for this as we could pre-sort by shader/texture/mesh
  Entity* AddEntity();
  void RemoveEntity(Entity* pEntity);

  // ugh this is all wrong, not going to be shader sorted, etc
  // but let's just do the stupid thing first
  void RenderEntitiesStupidly();

};

} // namespace fd