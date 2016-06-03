#pragma once

#include "../../common/component.h"
#include "../../common/fourmath.h"

#include "../stencil_portal.h"

namespace fd {

class Scene;
class Mesh;

class StencilPortalComponent : public Component {
public:
  int m_renderRecursionMax;
  int m_renderRecursionCurrent;
  int m_stencilMask;
  //int m_cachedStencilMask;

  Mat4f* m_pOwnerOrientation; // note as this is 4d we need full 4d matrix for orientation
  Vec4f* m_pOwnerPosition; // also need full vec4 for position.
  Scene* m_pOwnerScene;

  typedef std::vector<Camera*> TCameraStack;
  TCameraStack m_cameraStack;
  int m_nextCameraCopyIndex;
  Mat4f* m_targetOrientation; // note as this is 4d we need full 4d matrix for orientation
  Vec4f* m_targetPosition; // also need full vec4 for position.
  
public:
  StencilPortalComponent();
  ~StencilPortalComponent();
  virtual void OnConnected();
  virtual void OnAfterRender(Camera* pCamera, Render* pRender);
  virtual void OnBeforeRender(Camera* pCamera, Render* pRender);
};

}; //namespace fd

//#include <vector>
//
//#include "shader.h"
//#include "scene.h"
//
//namespace fd {
//
//class Scene;
//// this should be part of scene already?
//class SceneLoaderInterface {
//  virtual Scene* LoadASceneFrom(Scene* requestingScene) { return requestingScene->RecursiveClone(); }
//};
//
//class StencilPortal {
//public:
//  class DependentScene {
//    Scene* _pLoaded;
//  public:
//    DependentScene() : _pLoaded(NULL) {}
//  };
//  typedef std::vector<DependentScene*> TDepScenes;
//
//  //static 
//
//
//
//  //TDepScenes
//};
//
//}; // namespace fd
//
