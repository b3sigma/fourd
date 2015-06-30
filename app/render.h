#pragma once

#include <vector>
#include <tuple>
#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#include "fourmath.h"
#include "camera.h"
#include "timer.h"

namespace fd {

class Camera;
class Scene;
class Shader;
class Texture;

// Render should contain all the GL code
// View will contain a specific render target and a camera

class Input {
  // Wow this is so useful. Good thing it's here.
};


//The view is abstracted from the camera as it may be useful
//to render the same camera and scene onto multiple render targets.
//Many of the fields could plausibly go in either structure.
class View {
  Camera* m_camera;
  Scene* m_scene;
  Texture* m_renderTargetColor; // may be null to indicate backbuffer
  Texture* m_renderTargetDepth; // may be null to indicate backbuffer
};

class Render {
  //TODO: Actually convert to a view system
  //typedef std::vector<View*> TViewList;
  //TViewList _views;

  typedef std::vector<Camera*> TCameraList;
  TCameraList m_cameras; //not owned

  typedef std::vector<Scene*> TSceneList;
  TSceneList m_scenes; // not owned

  Shader* m_pOverdrawQuaxol;
  Shader* m_pSlicedQuaxol;
  Shader* m_pComposeRenderTargets;

  // should roll this stuff into view?
  Texture* m_overdrawColor;
  Texture* m_overdrawDepth; // dunno why this is needed
  Texture* m_renderColor;
  Texture* m_renderDepth;
  
  // shouldn't be here..
  // should be in a scene or something?
  ::fd::Timer timer_;
  double _lastTotalTime;
  double _frameTime;
  
public:  
  Vec4f m_clearColor;
  bool m_multiPass;
  Vec4f m_sliceRange;


public:
  Render();
  ~Render();

  bool Initialize(int width, int height);
  bool ResizeRenderTargets(int width, int height);

  void UpdateFrameTime();
  void Step();
  double GetFrameTime();
  double GetTotalTime();

  void AddCamera(Camera* pCamera);
  void AddScene(Scene* pScene);

  void RenderAllScenesPerCamera(Texture* pRenderColor, Texture* pRenderDepth);
  void RenderScene(Camera* pCamera, Scene* pScene, Texture* pRenderColor, Texture* pRenderDepth);

  // Right now this is convenient, but separate calls are fine too.
  enum EAlphaDepthModes {
    AlphaOnDepthOffSrcDest,
    AlphaOnDepthOffAdditive,
    AlphaTestDepthOffSrcDest,
    AlphaTestDepthOnSrcDest,
    AlphaOffDepthOn,
    ENumAlphaDepthModes,
    EToggleModes
  };
  EAlphaDepthModes m_alphaDepthMode = ENumAlphaDepthModes;

  void ToggleAlphaDepthModes(EAlphaDepthModes mode = EToggleModes);

  struct ComposeVert {
    float x, y;
    //float u, v;
  };
  ComposeVert m_composeVerts[6];
  bool InitializeComposeVerts();
  void RenderCompose(Texture* pDestination, 
      Texture* pColorTarget, Texture* pOverdrawSource);
  void ToggleMultipassMode(bool multiPass, int width, int height);
};

}  // namespace fd
