#include "scene.h"
#include "entity.h"

namespace fd {

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
  for(auto pEntity : m_dynamicEntities) {
    
  }
}

} // namespace fd
