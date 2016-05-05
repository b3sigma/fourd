#pragma once
#include <vector>

#include "shader.h"
#include "scene.h"

namespace fd {

class Scene;
// this should be part of scene already?
class SceneLoaderInterface {
  virtual Scene* LoadASceneFrom(Scene* requestingScene) { return requestingScene->RecursiveClone(); }
};

class StencilPortal {
public:
  class DependentScene {
    Scene* _pLoaded;
  public:
    DependentScene() : _pLoaded(NULL) {}
  };
  typedef std::vector<DependentScene*> TDepScenes;

  //static 



  //TDepScenes
};

}; // namespace fd

